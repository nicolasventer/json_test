#include "IModule.hpp"
#include "Timer.hpp"
#include "device_logic.hpp"
#include "test_module.hpp"

#include <gtest/gtest.h>

#include <chrono>

#define ABS_DIFF(a, b) (((a) >= (b)) ? ((a) - (b)) : ((b) - (a)))

namespace {

class SuiteDispositif : public testing::Test {
 protected:
  DeviceLogic logic;
  TestModule module{logic, true};

  void SetUp() override { module.Start(); }
};

TEST_F(SuiteDispositif, DemarrageDuDispositif) {
  module.expect.clear();

  // Réglage de l'intervalle et du seuil
  ConfigStatus data_in_1{};
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceiveDeviceConfigStatus(data_in_1);

  CmdCommand data_in_2{};
  data_in_2.value = "START";
  logic.onReceiveDeviceCmdCommand(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(100));

  // Le dispositif tourne depuis au moins 100 ms
  const StatusStatus* data_out_1 = module.expect.find_as<StatusStatus>("/device/status/status");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"/device/status/status\" envoyé";
  EXPECT_EQ(data_out_1->status, EStateStatus::RUNNING);
  EXPECT_GE(data_out_1->uptime_ms, 100);

  const StatusStatus* data_out_2 = module.expect.find_as<StatusStatus>("/device/status/status");
  ASSERT_NE(data_out_2, nullptr) << "aucun message \"/device/status/status\" envoyé";
  EXPECT_LE(ABS_DIFF(data_out_2->uptime_ms, 100), 20);

  module.expect.clear();
  const AlarmStatus* data_out_3 = module.expect.find_as<AlarmStatus>("/device/alarm/status");
  EXPECT_EQ(data_out_3, nullptr) << "un message \"/device/alarm/status\" inattendu";
}

TEST_F(SuiteDispositif, MesureSousLeSeuil) {
  module.expect.clear();

  ConfigStatus data_in_1{};
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceiveDeviceConfigStatus(data_in_1);

  CmdCommand data_in_2{};
  data_in_2.value = "START";
  logic.onReceiveDeviceCmdCommand(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(100));

  const MeasureStatus* data_out_1 = module.expect.find_as<MeasureStatus>("/device/measure/status");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"/device/measure/status\" envoyé";
  EXPECT_GE(data_out_1->temperature, 20);
  EXPECT_LT(data_out_1->humidity, 80);

  const MeasureStatus* data_out_2 = module.expect.find_as<MeasureStatus>("/device/measure/status");
  ASSERT_NE(data_out_2, nullptr) << "aucun message \"/device/measure/status\" envoyé";
  EXPECT_LT(data_out_2->temperature, 30.5);
}

TEST_F(SuiteDispositif, SeuilDepasse) {
  module.expect.clear();

  ConfigStatus data_in_1{};
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceiveDeviceConfigStatus(data_in_1);

  CmdCommand data_in_2{};
  data_in_2.value = "START";
  logic.onReceiveDeviceCmdCommand(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(200));

  const AlarmStatus* data_out_1 = module.expect.find_as<AlarmStatus>("/device/alarm/status");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"/device/alarm/status\" envoyé";
  EXPECT_EQ(data_out_1->level, EAlarmLevel::WARN);
  EXPECT_EQ(data_out_1->code, 42);

  const StatusStatus* data_out_2 = module.expect.find_as<StatusStatus>("/device/status/status");
  ASSERT_NE(data_out_2, nullptr) << "aucun message \"/device/status/status\" envoyé";
  EXPECT_EQ(data_out_2->status, EStateStatus::CLOSED);
}

TEST_F(SuiteDispositif, ArretDuDispositif) {
  module.expect.clear();

  ConfigStatus data_in_1{};
  data_in_1.interval_ms = 100;
  data_in_1.threshold = 30.5;
  logic.onReceiveDeviceConfigStatus(data_in_1);

  CmdCommand data_in_2{};
  data_in_2.value = "START";
  logic.onReceiveDeviceCmdCommand(data_in_2);

  module.getTimer().sleep(std::chrono::milliseconds(100));

  CmdCommand data_in_3{};
  data_in_3.value = "STOP";
  logic.onReceiveDeviceCmdCommand(data_in_3);

  const StatusStatus* data_out_1 = module.expect.find_as<StatusStatus>("/device/status/status");
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"/device/status/status\" envoyé";
  EXPECT_EQ(data_out_1->status, EStateStatus::CLOSED);
}

TEST_F(SuiteDispositif, TrameMoteur) {
  module.expect.clear();

  // Vitesse sur les bits à partir de start
  // { "id": "ENGINE", "start": 4, "data": "1011" }
  CanMessageCommand data_in_1{};
  data_in_1.id_code = 256u;
  data_in_1.data = 0xB0ULL;
  logic.onReceiveDeviceCanMessageCommand(data_in_1);

  // { "id": "ENGINE", "start": 4, "data": [">=", "1011"] }
  const CanMessageStatus* data_out_1 = module.expect.find_as<CanMessageStatus>(std::string("/device/can_message/status/") + std::to_string(256));
  ASSERT_NE(data_out_1, nullptr) << "aucun message \"ENGINE\" envoyé";
  EXPECT_EQ(data_out_1->id_code, 256u);
  EXPECT_GE(data_out_1->data, 0xB0ULL);

  // { "id": "ENGINE", "start": 4, "data": ["~=", "1011", 0] }
  const CanMessageStatus* data_out_2 = module.expect.find_as<CanMessageStatus>(std::string("/device/can_message/status/") + std::to_string(256));
  ASSERT_NE(data_out_2, nullptr) << "aucun message \"ENGINE\" envoyé";
  EXPECT_EQ(data_out_2->id_code, 256u);
  EXPECT_LE(ABS_DIFF(data_out_2->data, 0xB0ULL), 0ULL);

  module.expect.clear();
  // { "id": "ALARM" }
  const CanMessageStatus* data_out_3 = module.expect.find_as<CanMessageStatus>(std::string("/device/can_message/status/") + std::to_string(512));
  EXPECT_EQ(data_out_3, nullptr) << "un message \"ALARM\" inattendu";
}

}  // namespace
