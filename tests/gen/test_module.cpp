#include "test_module.hpp"

TestModule::TestModule(IModuleLogic& logic, bool useVirtualTime)
    : logic_(logic), use_virtual_time_(useVirtualTime) {
  logic_.setModule(*this);
}

Timer& TestModule::getTimer() { return timer_; }

IModuleLogic& TestModule::getLogic() { return logic_; }

void TestModule::Start() {
  if (use_virtual_time_) {
    timer_.startVirtual();
  } else {
    timer_.startReal();
  }
}

void TestModule::sendDeviceStatusStatus(const StatusStatus& message) {
  expect["/device/status/status"] = message;
}

void TestModule::sendDeviceMeasureStatus(const MeasureStatus& message) {
  expect["/device/measure/status"] = message;
}

void TestModule::sendDeviceAlarmStatus(const AlarmStatus& message) {
  expect["/device/alarm/status"] = message;
}

void TestModule::sendDeviceCanMessageStatus(const CanMessageStatus& message) {
  expect[std::string("/device/can_message/status/") + std::to_string(message.id_code)] = message;
}
