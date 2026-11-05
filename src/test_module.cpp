#include "test_module.hpp"

#include "module_json.hpp"

#include <utility>

namespace json_test {

TestModule::TestModule(Timer& timer) : timer_(timer) {}

void TestModule::setLogic(IModuleLogic* logic) { logic_ = logic; }

void TestModule::Start() { timer_.startVirtual(); }

void TestModule::send(state message) { record("state", jw::to_value(message)); }

void TestModule::send(measure message) { record("measure", jw::to_value(message)); }

void TestModule::send(alarm message) { record("alarm", jw::to_value(message)); }

void TestModule::resetExpect() { reset_index_ = sent_.size(); }

const std::vector<IModuleExpect::Sent>& TestModule::sent() const { return sent_; }

std::size_t TestModule::resetIndex() const { return reset_index_; }

void TestModule::record(const std::string& topic, jw::Value data) {
  sent_.push_back(Sent{topic, std::move(data)});
}

}  // namespace json_test
