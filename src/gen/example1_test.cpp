#include "IModule.hpp"
#include "Timer.hpp"
#include "device_logic.hpp"
#include "test_module.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace {

class SuiteDispositif : public testing::Test {
 protected:
  DeviceLogic logic;
  TestModule module{logic, true};

  void SetUp() override { module.Start(); }
};

TEST_F(SuiteDispositif, DemarrageDuDispositif) {
  module.expect.clear();

  config data_in_1;
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceive_config(data_in_1);

  cmd data_in_2;
  data_in_2.value = "START";
  logic.onReceive_cmd(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(100));

  const state* data_out_1 = module.expect.find_as<state>("state");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"state\" envoyé";
  EXPECT_EQ(data_out_1->status, EStateStatus::RUNNING);
  EXPECT_GE(data_out_1->uptime_ms, 100);

  module.expect.clear();
  const alarm* data_out_2 = module.expect.find_as<alarm>("alarm");
  EXPECT_EQ(data_out_2, nullptr) << "un message \"alarm\" inattendu";
}

TEST_F(SuiteDispositif, MesureSousLeSeuil) {
  module.expect.clear();

  config data_in_1;
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceive_config(data_in_1);

  cmd data_in_2;
  data_in_2.value = "START";
  logic.onReceive_cmd(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(100));

  const measure* data_out_1 = module.expect.find_as<measure>("measure");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"measure\" envoyé";
  EXPECT_GE(data_out_1->temperature, 20);
  EXPECT_LT(data_out_1->humidity, 80);

  const measure* data_out_2 = module.expect.find_as<measure>("measure");
  ASSERT_NE(data_out_2, nullptr) << "aucun message \"measure\" envoyé";
  EXPECT_LT(data_out_2->temperature, 30.5);
}

TEST_F(SuiteDispositif, SeuilDepasse) {
  module.expect.clear();

  config data_in_1;
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceive_config(data_in_1);

  cmd data_in_2;
  data_in_2.value = "START";
  logic.onReceive_cmd(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(200));

  const alarm* data_out_1 = module.expect.find_as<alarm>("alarm");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"alarm\" envoyé";
  EXPECT_EQ(data_out_1->level, EAlarmLevel::WARN);
  EXPECT_EQ(data_out_1->code, 42);

  const state* data_out_2 = module.expect.find_as<state>("state");
  ASSERT_NE(data_out_2, nullptr) << "aucun message \"state\" envoyé";
  EXPECT_EQ(data_out_2->status, EStateStatus::CLOSED);
}

TEST_F(SuiteDispositif, ArretDuDispositif) {
  module.expect.clear();

  config data_in_1;
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceive_config(data_in_1);

  cmd data_in_2;
  data_in_2.value = "START";
  logic.onReceive_cmd(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(100));

  cmd data_in_3;
  data_in_3.value = "STOP";
  logic.onReceive_cmd(data_in_3);

  const state* data_out_1 = module.expect.find_as<state>("state");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"state\" envoyé";
  EXPECT_EQ(data_out_1->status, EStateStatus::CLOSED);
}

}  // namespace
