#include "console_module.hpp"

#include <iostream>

ConsoleModule::ConsoleModule(IModuleLogic& logic) : logic_(logic) { logic_.setModule(*this); }

Timer& ConsoleModule::getTimer() { return timer_; }

IModuleLogic& ConsoleModule::getLogic() { return logic_; }

void ConsoleModule::Start() { timer_.startReal(); }

void ConsoleModule::send_state(const state& message) {
  std::cout << "state " << message.status << ' ' << message.uptime_ms << std::endl;
}

void ConsoleModule::send_measure(const measure& message) {
  std::cout << "measure " << message.temperature << ' ' << message.humidity << std::endl;
}

void ConsoleModule::send_alarm(const alarm& message) {
  std::cout << "alarm " << message.level << ' ' << message.code << std::endl;
}
