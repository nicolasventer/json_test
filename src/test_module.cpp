#include "test_module.hpp"

#include <utility>

namespace json_test {

TestModule::TestModule(IModuleLogic& logic) : logic_(logic) { logic_.setModule(*this); }

Timer& TestModule::getTimer() { return timer_; }

IModuleLogic& TestModule::getLogic() { return logic_; }

void TestModule::Start() { timer_.startVirtual(); }

void TestModule::send(state message) { expect.record("state", std::move(message)); }

void TestModule::send(measure message) { expect.record("measure", std::move(message)); }

void TestModule::send(alarm message) { expect.record("alarm", std::move(message)); }

}  // namespace json_test
