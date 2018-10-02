#include <gtest/gtest.h>

#include <curl/curl.h>
#include <thread>
#include "ostree_http_repo.h"
#include "ostree_ref.h"
#include "test_utils.h"
std::string port;

static size_t writeString(void *contents, size_t size, size_t nmemb, void *userp) {
  assert(userp);
  // append the writeback data to the provided string
  (static_cast<std::string *>(userp))->append(static_cast<char *>(contents), size * nmemb);

  // return size of written data
  return size * nmemb;
}

TEST(treehub_server, token_auth) {
  TreehubServer server;
  CurlEasyWrapper curl_handle;
  server.root_url(std::string("http://127.0.0.1:") + port);
  server.SetToken("test_token");
  server.InjectIntoCurl("/", curl_handle.get());
  std::string response;
  curlEasySetoptWrapper(curl_handle.get(), CURLOPT_WRITEFUNCTION, writeString);
  curlEasySetoptWrapper(curl_handle.get(), CURLOPT_WRITEDATA, static_cast<void *>(&response));
  curl_easy_perform(curl_handle.get());

  long rescode;  // NOLINT
  curl_easy_getinfo(curl_handle.get(), CURLINFO_RESPONSE_CODE, &rescode);
  if (rescode == 200) {
    auto response_json = Utils::parseJSON(response);
    EXPECT_EQ(response_json["Authorization"], "Bearer test_token");
  } else {
    FAIL();
  }
}

TEST(treehub_server, basic_auth) {
  TreehubServer server;
  CurlEasyWrapper curl_handle;
  server.root_url(std::string("http://127.0.0.1:") + port);
  server.SetAuthBasic("login", "password");
  server.InjectIntoCurl("/", curl_handle.get());
  std::string response;
  curlEasySetoptWrapper(curl_handle.get(), CURLOPT_WRITEFUNCTION, writeString);
  curlEasySetoptWrapper(curl_handle.get(), CURLOPT_WRITEDATA, static_cast<void *>(&response));
  curl_easy_perform(curl_handle.get());

  long rescode;  // NOLINT
  curl_easy_getinfo(curl_handle.get(), CURLINFO_RESPONSE_CODE, &rescode);
  if (rescode == 200) {
    auto response_json = Utils::parseJSON(response);
    EXPECT_EQ(response_json["Authorization"], "Basic bG9naW46cGFzc3dvcmQ=");
  } else {
    FAIL();
  }
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  std::string server = "tests/sota_tools/headers_response_server.py";
  port = TestUtils::getFreePort();

  TestHelperProcess server_process(server, port);
  sleep(3);

  return RUN_ALL_TESTS();
}
#endif

// vim: set tabstop=2 shiftwidth=2 expandtab:
