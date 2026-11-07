#pragma once

#include "IModule.hpp"
#include "Timer.hpp"
#include "module_expect.hpp"

namespace json_test {

class TestModule : public IModule {
 public:
  ModuleExpect<state, measure, alarm> expect;

  TestModule(IModuleLogic& logic, bool useVirtualTime);

  Timer& getTimer() override;
  IModuleLogic& getLogic() override;
  void Start() override;

  void send_state(const state& message) override;
  void send_measure(const measure& message) override;
  void send_alarm(const alarm& message) override;

 private:
  Timer timer_;
  IModuleLogic& logic_;
  bool use_virtual_time_;
};

}  // namespace json_test
