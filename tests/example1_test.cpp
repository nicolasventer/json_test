#include "IModule.hpp"
#include "Timer.hpp"
#include "device_logic.hpp"
#include "test_module.hpp"

#include <gtest/gtest.h>

#include <chrono>

namespace {

class SuiteDispositif : public testing::Test {
 protected:
  Timer timer;
  json_test::TestModule module{timer};
  json_test::DeviceLogic logic{module, timer};

  void SetUp() override {
    module.setLogic(&logic);
    module.Start();
  }
};

TEST_F(SuiteDispositif, DemarrageDuDispositif) {
  module.expect.resetExpect();
  logic.onReceive(config{100, 30.5});
  logic.onReceive(cmd{"START"});
  timer.sleep(std::chrono::milliseconds{100});

  const state* data_1 = module.expect.rFindSend<state>("state");
  ASSERT_NE(data_1, nullptr) << "aucun message \"state\" envoyé";
  EXPECT_EQ(data_1->status, RUNNING);
  EXPECT_EQ(data_1->uptime_ms, 100);

  module.expect.resetExpect();
  const alarm* data_2 = module.expect.rFindSend<alarm>("alarm");
  ASSERT_EQ(data_2, nullptr) << "un message \"alarm\" inattendu";
}

TEST_F(SuiteDispositif, MesureSousLeSeuil) {
  module.expect.resetExpect();
  logic.onReceive(config{100, 30.5});
  logic.onReceive(cmd{"START"});
  timer.sleep(std::chrono::milliseconds{100});

  const measure* data_1 = module.expect.rFindSend<measure>("measure");
  ASSERT_NE(data_1, nullptr) << "aucun message \"measure\" envoyé";
  EXPECT_GE(data_1->temperature, 20.0);
  EXPECT_LT(data_1->humidity, 80);

  const measure* data_2 = module.expect.rFindSend<measure>("measure");
  ASSERT_NE(data_2, nullptr) << "aucun message \"measure\" envoyé";
  EXPECT_LT(data_2->temperature, 30.5);
}

TEST_F(SuiteDispositif, SeuilDepasse) {
  module.expect.resetExpect();
  logic.onReceive(config{100, 30.5});
  logic.onReceive(cmd{"START"});
  timer.sleep(std::chrono::milliseconds{200});

  const alarm* data_1 = module.expect.rFindSend<alarm>("alarm");
  ASSERT_NE(data_1, nullptr) << "aucun message \"alarm\" envoyé";
  EXPECT_EQ(data_1->level, WARN);
  EXPECT_EQ(data_1->code, 42);

  const state* data_2 = module.expect.rFindSend<state>("state");
  ASSERT_NE(data_2, nullptr) << "aucun message \"state\" envoyé";
  EXPECT_EQ(data_2->status, CLOSED);
}

TEST_F(SuiteDispositif, ArretDuDispositif) {
  module.expect.resetExpect();
  logic.onReceive(config{100, 30.5});
  logic.onReceive(cmd{"START"});
  timer.sleep(std::chrono::milliseconds{100});
  logic.onReceive(cmd{"STOP"});

  const state* data_1 = module.expect.rFindSend<state>("state");
  ASSERT_NE(data_1, nullptr) << "aucun message \"state\" envoyé";
  EXPECT_EQ(data_1->status, CLOSED);
}

}  // namespace
