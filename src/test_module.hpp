#pragma once

#include "IModule.hpp"
#include "Timer.hpp"
#include "module_expect.hpp"

namespace json_test {

class TestModule : public IModule {
 public:
  ModuleExpect<state, measure, alarm> expect;

  explicit TestModule(IModuleLogic& logic);

  Timer& getTimer() override;
  IModuleLogic& getLogic() override;
  void Start() override;

  void send(state message) override;
  void send(measure message) override;
  void send(alarm message) override;

 private:
  Timer timer_;
  IModuleLogic& logic_;
};

}  // namespace json_test
