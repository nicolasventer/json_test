#include "console_module.hpp"

#include <iostream>

ConsoleModule::ConsoleModule(IModuleLogic& logic) : logic_(logic) { logic_.setModule(*this); }

Timer& ConsoleModule::getTimer() { return timer_; }

IModuleLogic& ConsoleModule::getLogic() { return logic_; }

void ConsoleModule::Start() { timer_.startReal(); }

void ConsoleModule::sendDeviceStatusStatus(const StatusStatus& message) {
  std::cout << "status " << message.status << ' ' << message.uptime_ms << std::endl;
}

void ConsoleModule::sendDeviceMeasureStatus(const MeasureStatus& message) {
  std::cout << "measure " << message.temperature << ' ' << message.humidity << std::endl;
}

void ConsoleModule::sendDeviceAlarmStatus(const AlarmStatus& message) {
  std::cout << "alarm " << message.level << ' ' << message.code << std::endl;
}

void ConsoleModule::sendDeviceCanMessageStatus(const CanMessageStatus& message) {
  std::cout << "can " << message.id_code << ' ' << message.data << std::endl;
}
