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

void TestModule::send_state(const state& message) { expect.record("state", message); }

void TestModule::send_measure(const measure& message) { expect.record("measure", message); }

void TestModule::send_alarm(const alarm& message) { expect.record("alarm", message); }
