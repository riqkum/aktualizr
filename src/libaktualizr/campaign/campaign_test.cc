#include <boost/filesystem.hpp>

#include "campaign/campaign.h"

#include <gtest/gtest.h>

boost::filesystem::path test_data_dir;

TEST(campaign, Campaigns_from_json) {
  auto json = Utils::parseJSONFile(test_data_dir / "campaigns_sample.json");

  auto campaigns = campaign::campaignsFromJson(json);
  EXPECT_EQ(campaigns.size(), 1);

  EXPECT_EQ(campaigns.at(0).name, "campaign1");
  EXPECT_EQ(campaigns.at(0).id, "c2eb7e8d-8aa0-429d-883f-5ed8fdb2a493");
  EXPECT_TRUE(campaigns.at(0).autoAccept);
  EXPECT_EQ(campaigns.at(0).description, "this is my message to show on the device");
  EXPECT_EQ(campaigns.at(0).estInstallationDuration, 10);
  EXPECT_EQ(campaigns.at(0).estPreparationDuration, 20);

  // legacy: no autoAccept
  Json::Value bad4;
  bad4["campaigns"] = Json::Value(Json::arrayValue);
  bad4["campaigns"][0] = Json::Value();
  bad4["campaigns"][0]["name"] = "a";
  bad4["campaigns"][0]["id"] = "a";
  auto campaignsNoAutoAccept = campaign::campaignsFromJson(bad4);
  EXPECT_FALSE(campaignsNoAutoAccept.at(0).autoAccept);
}

TEST(campaign, Campaigns_from_invalid_json) {
  // empty object
  EXPECT_EQ(campaign::campaignsFromJson(Json::Value()).size(), 0);

  // naked array
  EXPECT_EQ(campaign::campaignsFromJson(Json::Value(Json::arrayValue)).size(), 0);

  // object in object
  Json::Value bad1;
  bad1["campaigns"] = Json::Value();
  EXPECT_EQ(campaign::campaignsFromJson(bad1).size(), 0);

  // array in array in object
  Json::Value bad2;
  bad2["campaigns"] = Json::Value(Json::arrayValue);
  bad2["campaigns"][0] = Json::Value(Json::arrayValue);
  EXPECT_EQ(campaign::campaignsFromJson(bad2).size(), 0);

  // no name
  Json::Value bad3;
  bad3["campaigns"] = Json::Value(Json::arrayValue);
  bad3["campaigns"][0] = Json::Value();
  EXPECT_EQ(campaign::campaignsFromJson(bad3).size(), 0);
}

#ifndef __NO_MAIN__
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  if (argc != 2) {
    std::cerr << "Error: " << argv[0] << " requires the path to the test data as an input argument.\n";
    return EXIT_FAILURE;
  }
  test_data_dir = argv[1];

  return RUN_ALL_TESTS();
}
#endif
