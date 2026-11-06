#include "test_module.hpp"

#include <utility>

namespace json_test {

TestModule::TestModule(Timer& timer) : timer_(timer) {}

void TestModule::setLogic(IModuleLogic* logic) { logic_ = logic; }

void TestModule::Start() { timer_.startVirtual(); }

void TestModule::send(state message) { expect.record("state", std::move(message)); }

void TestModule::send(measure message) { expect.record("measure", std::move(message)); }

void TestModule::send(alarm message) { expect.record("alarm", std::move(message)); }

}  // namespace json_test
