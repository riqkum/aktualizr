#!/usr/bin/env python3

import argparse
import os
import re
import sys
import shutil
import tempfile

from pathlib import Path
from time import sleep

from prov_test_common import popen_subprocess, run_subprocess, verify_provisioned


def main():
    parser = argparse.ArgumentParser(description='Run a local implicit provisioning test with aktualizr')
    parser.add_argument('--build-dir', '-b', type=Path, default=Path('../build'), help='build directory')
    parser.add_argument('--src-dir', '-s', type=Path, default=Path('../'), help='source directory (parent of src/)')
    parser.add_argument('--credentials', '-c', type=Path, default=Path('.'), help='path to credentials archive')
    parser.add_argument('--pkcs11-module', '-p', type=Path, default=Path('/usr/lib/softhsm/libsofthsm2.so'), help='path to PKCS#11 library module')
    args = parser.parse_args()

    retval = 1
    with tempfile.TemporaryDirectory() as tmp_dir:
        retval = provision(Path(tmp_dir), args.build_dir, args.src_dir,
                args.credentials, args.pkcs11_module)
    return retval


CONFIG_TEMPLATE = '''
[pacman]
type = "none"

[tls]
cert_source = "pkcs11"
pkey_source = "pkcs11"

[p11]
module = "{pkcs11_module}"
pass = "1234"
uptane_key_id = "03"
tls_clientcert_id = "01"
tls_pkey_id = "02"

[uptane]
key_source = "pkcs11"

[storage]
type = "sqlite"
path = "{tmp_dir}"
tls_cacert_path = "token/root.crt"
tls_clientcert_path = "token/client.pem"
tls_pkey_path = "token/pkey.pem"

[import]
base_path = "{tmp_dir}/certs"
tls_cacert_path = "root.crt"
tls_clientcert_path = "client.pem"
tls_pkey_path = "pkey.pem"
'''


def provision(tmp_dir, build_dir, src_dir, creds, pkcs11_module):
    conf_dir = tmp_dir / 'conf.d'
    os.mkdir(str(conf_dir))
    conf_prov = conf_dir / '20-sota_hsm_prov.toml'
    conf_server = conf_dir / '30-implicit_server.toml'
    with conf_prov.open('w') as f:
        f.write(CONFIG_TEMPLATE.format(tmp_dir=tmp_dir, pkcs11_module=pkcs11_module))
    akt = build_dir / 'src/aktualizr_primary/aktualizr'
    akt_info = build_dir / 'src/aktualizr_info/aktualizr-info'
    akt_iw = build_dir / 'src/implicit_writer/aktualizr_implicit_writer'
    akt_cp = build_dir / 'src/cert_provider/aktualizr_cert_provider'
    setup_hsm = src_dir / 'scripts/export_to_hsm.sh'
    hsm_conf = tmp_dir / 'softhsm2.conf'
    token_dir = tmp_dir / 'token'
    certs_dir = tmp_dir / 'certs'


    # Run implicit_writer (equivalent to aktualizr-hsm-prov.bb).
    stdout, stderr, retcode = run_subprocess([str(akt_iw),
                                              '-c', str(creds),
                                              '--no-root-ca', '-o', str(conf_server)])
    if retcode > 0:
        print('aktualizr_implicit_writer failed (' + str(retcode) + '): ' +
              stderr.decode() + stdout.decode())
        return retcode

    os.environ['TOKEN_DIR'] = str(token_dir)
    os.environ['SOFTHSM2_CONF'] = str(hsm_conf)
    os.environ['CERTS_DIR'] = str(certs_dir)
    shutil.copyfile(str(src_dir / "tests/test_data/softhsm2.conf"), str(hsm_conf))

    with popen_subprocess([str(akt), '--config', str(conf_dir), '--loglevel', '0', '--running-mode', 'once']) as proc:
        try:
            # Verify that device has NOT yet provisioned.
            for delay in [1, 2, 5, 10, 15]:
                sleep(delay)
                stdout, stderr, retcode = run_subprocess([str(akt_info),
                                                          '--config', str(conf_dir)])
                if retcode == 0 and stderr == b'':
                    break
            if (b'Couldn\'t load device ID' not in stdout or
                    b'Couldn\'t load ECU serials' not in stdout or
                    b'Provisioned on server: no' not in stdout or
                    b'Fetched metadata: no' not in stdout):
                print('Error: aktualizr failure or device already provisioned: \n' + stderr.decode() + stdout.decode())
                return 1
        finally:
            proc.kill()

    # Unlike in meta-updater's oe-selftest, don't check if the HSM is already
    # initialized. If setup_hsm.sh was already run, it *should* be initialized.
    # If we run it from this script below, the script will reset where the
    # tokens are stored, and until that happens, we would be checking the wrong
    # directory.

    # Run cert_provider.
    print('Device has not yet provisioned (as expected). Running cert_provider.')
    stdout, stderr, retcode = run_subprocess([str(akt_cp),
        '-c', str(creds), '-l', '/', '-r', '-s', '-g', str(conf_prov)])
    if retcode > 0:
        print('aktualizr_cert_provider failed (' + str(retcode) + '): ' +
              stderr.decode() + stdout.decode())
        return retcode
    stdout, stderr, retcode = run_subprocess([str(setup_hsm)])
    if retcode > 0:
        print('setup_hsm.sh failed: ' + stdout.decode() + stderr.decode())
        return 1

    # Verify that HSM is able to initialize.
    pkcs11_command = ['pkcs11-tool', '--module=' + str(pkcs11_module), '-O',
                     '--login', '--pin', '1234']
    softhsm2_command = ['softhsm2-util', '--show-slots']

    p11_out, p11_err, p11_ret = run_subprocess(pkcs11_command)
    hsm_out, hsm_err, hsm_ret = run_subprocess(softhsm2_command)

    if not (p11_ret == 0 and hsm_ret == 0 and
            b'present token' in p11_err and
            b'X.509 cert' in p11_out and
            b'Initialized:      yes' in hsm_out and
            b'User PIN init.:   yes' in hsm_out and
            hsm_err == b''):
        print('pkcs11-tool or softhsm2-tool failed: ' + p11_err.decode() +
              p11_out.decode() + hsm_err.decode() + hsm_out.decode())
        return 1

    # Check that pkcs11 output matches sofhsm output.
    p11_p = re.compile(r'Using slot [0-9] with a present token \((0x[0-9a-f]*)\)\s')
    p11_m = p11_p.search(p11_err.decode())
    if not p11_m or p11_m.lastindex <= 0:
        print('Slot number not found with pkcs11-tool: ' + p11_err.decode() + p11_out.decode())
        return 1
    hsm_p = re.compile(r'Description:\s*SoftHSM slot ID (0x[0-9a-f]*)\s')
    hsm_m = hsm_p.search(hsm_out.decode())
    if not hsm_m or hsm_m.lastindex <= 0:
        print('Slot number not found with softhsm2-tool: ' + hsm_err.decode() + hsm_out.decode())
        return 1
    if p11_m.group(1) != hsm_m.group(1):
        print('Slot number does not match: ' + p11_err.decode() + p11_out.decode() +
              hsm_err.decode() + hsm_out.decode())
        return 1

    r = 1
    (tmp_dir / 'sql.db').unlink()
    with popen_subprocess([str(akt), '--config', str(conf_dir), '--loglevel', '0', '--running-mode', 'once']) as proc:
        try:
            r = verify_provisioned(akt_info, conf_dir)
        finally:
            proc.kill()
    return r


if __name__ == '__main__':
    sys.exit(main())
