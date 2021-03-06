#include <gtest/gtest.h>

#include "aktualizr_secondary.h"
#include "uptane/secondaryinterface.h"

#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>

#include "config/config.h"
#include "storage/invstorage.h"
#include "utilities/utils.h"
std::string sysroot;

class ShortCircuitSecondary : public Uptane::SecondaryInterface {
 public:
  ShortCircuitSecondary(const Uptane::SecondaryConfig& sconfig_in, AktualizrSecondary& sec)
      : SecondaryInterface(sconfig_in), secondary(sec) {}
  virtual ~ShortCircuitSecondary() {}

  virtual Uptane::EcuSerial getSerial() { return secondary.getSerialResp(); }
  virtual Uptane::HardwareIdentifier getHwId() { return secondary.getHwIdResp(); }
  virtual PublicKey getPublicKey() { return secondary.getPublicKeyResp(); }
  virtual Json::Value getManifest() { return secondary.getManifestResp(); }
  virtual bool putMetadata(const Uptane::RawMetaPack& meta_pack) { return secondary.putMetadataResp(meta_pack); }
  virtual int32_t getRootVersion(bool director) { return secondary.getRootVersionResp(director); }
  virtual bool putRoot(const std::string& root, bool director) { return secondary.putRootResp(root, director); }
  virtual std::future<bool> sendFirmwareAsync(const std::shared_ptr<std::string>& data) {
    return std::async(std::launch::async, &AktualizrSecondary::sendFirmwareResp, &secondary, data);
  }

 private:
  AktualizrSecondary& secondary;
};

TEST(aktualizr_secondary_protocol, DISABLED_manual_update) {
  // secondary
  TemporaryDirectory temp_dir_sec;
  AktualizrSecondaryConfig config;
  config.network.port = 0;
  config.storage.type = StorageType::kSqlite;
  config.pacman.sysroot = sysroot;
  auto storage = INvStorage::newStorage(config.storage);

  AktualizrSecondary as(config, storage);

  // secondary interface
  Uptane::SecondaryConfig config_iface;
  ShortCircuitSecondary sec_iface{config_iface, as};

  // storage
  TemporaryDirectory temp_dir;
  Utils::copyDir("tests/test_data/secondary_meta", temp_dir.Path());
  StorageConfig storage2_config;
  storage2_config.path = temp_dir.Path();
  auto storage2 = INvStorage::newStorage(storage2_config);

  Uptane::RawMetaPack metadata;
  EXPECT_TRUE(storage2->loadLatestRoot(&metadata.director_root, Uptane::RepositoryType::Director));
  EXPECT_TRUE(
      storage2->loadNonRoot(&metadata.director_targets, Uptane::RepositoryType::Director, Uptane::Role::Targets()));
  EXPECT_TRUE(storage2->loadLatestRoot(&metadata.image_root, Uptane::RepositoryType::Images));
  EXPECT_TRUE(storage2->loadNonRoot(&metadata.image_targets, Uptane::RepositoryType::Images, Uptane::Role::Targets()));
  EXPECT_TRUE(
      storage2->loadNonRoot(&metadata.image_timestamp, Uptane::RepositoryType::Images, Uptane::Role::Timestamp()));
  EXPECT_TRUE(
      storage2->loadNonRoot(&metadata.image_snapshot, Uptane::RepositoryType::Images, Uptane::Role::Snapshot()));

  std::string firmware = Utils::readFile(temp_dir.Path() / "firmware.bin");

  EXPECT_TRUE(sec_iface.putMetadata(metadata));
  EXPECT_TRUE(sec_iface.sendFirmwareAsync(std::make_shared<std::string>(firmware)).get());
  Json::Value manifest = sec_iface.getManifest();

  EXPECT_EQ(manifest["signed"]["installed_image"]["fileinfo"]["hashes"]["sha256"],
            boost::algorithm::to_lower_copy(boost::algorithm::hex(Crypto::sha256digest(firmware))));
}

#ifndef __NO_MAIN__
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  if (argc != 2) {
    std::cerr << "Error: " << argv[0] << " requires the path to an OSTree sysroot as an input argument.\n";
    return EXIT_FAILURE;
  }
  sysroot = argv[1];
  return RUN_ALL_TESTS();
}
#endif
