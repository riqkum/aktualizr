#include <gtest/gtest.h>

#include "crypto/crypto.h"
#include "deploy.h"
#include "ostree_dir_repo.h"
#include "ostree_ref.h"
#include "test_utils.h"
std::string port = "2443";
TemporaryDirectory temp_dir;

TEST(deploy, UploadToTreehub) {
  OSTreeRepo::ptr src_repo = std::make_shared<OSTreeDirRepo>("tests/sota_tools/repo");
  boost::filesystem::path filepath = (temp_dir.Path() / "auth.json").string();
  boost::filesystem::path cert_path = "tests/fake_http_server/client.crt";

  const uint8_t hash[32] = {0x16, 0xef, 0x2f, 0x26, 0x29, 0xdc, 0x92, 0x63, 0xfd, 0xf3, 0xc0,
                            0xf0, 0x32, 0x56, 0x3a, 0x2d, 0x75, 0x76, 0x23, 0xbb, 0xc1, 0x1c,
                            0xf9, 0x9d, 0xf2, 0x5c, 0x3c, 0x3f, 0x25, 0x8d, 0xcc, 0xbe};
  UploadToTreehub(src_repo, ServerCredentials(filepath), OSTreeHash(hash), cert_path.string(), false, 2);

  EXPECT_EQ(
      Utils::readFile(temp_dir.Path() /
                      "objects/2a/28dac42b76c2015ee3c41cc4183bb8b5c790fd21fa5cfa0802c6e11fd0edbe.dirmeta"),
      Utils::readFile(std::string(
          "tests/sota_tools/repo/objects/2a/28dac42b76c2015ee3c41cc4183bb8b5c790fd21fa5cfa0802c6e11fd0edbe.dirmeta")));

  EXPECT_EQ(
      Utils::readFile(temp_dir.Path() /
                      "objects/a1/f4f81612ce959883f58e83789f6c7d97b0b55b801b2a09955235f40b0f2dfb.filez"),
      Utils::readFile(std::string(
          "tests/sota_tools/repo/objects/a1/f4f81612ce959883f58e83789f6c7d97b0b55b801b2a09955235f40b0f2dfb.filez")));

  EXPECT_EQ(
      Utils::readFile(temp_dir.Path() /
                      "objects/16/ef2f2629dc9263fdf3c0f032563a2d757623bbc11cf99df25c3c3f258dccbe.commit"),
      Utils::readFile(std::string(
          "tests/sota_tools/repo/objects/16/ef2f2629dc9263fdf3c0f032563a2d757623bbc11cf99df25c3c3f258dccbe.commit")));
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  port = TestUtils::getFreePort();
  std::string server = "tests/sota_tools/treehub_deploy_server.py";
  Json::Value auth;
  auth["ostree"]["server"] = std::string("https://localhost:") + port;
  Utils::writeFile(temp_dir.Path() / "auth.json", auth);

  TestHelperProcess server_process(server, port, temp_dir.PathString());
  sleep(3);
  return RUN_ALL_TESTS();
}
#endif

// vim: set tabstop=2 shiftwidth=2 expandtab:
