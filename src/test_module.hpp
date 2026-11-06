#pragma once

#include "IModule.hpp"
#include "Timer.hpp"
#include "module_expect.hpp"

namespace json_test {

class TestModule : public IModule {
 public:
  ModuleExpect<state, measure, alarm> expect;

  explicit TestModule(Timer& timer);

  void setLogic(IModuleLogic* logic) override;
  void Start() override;

  void send(state message) override;
  void send(measure message) override;
  void send(alarm message) override;

 private:
  Timer& timer_;
  IModuleLogic* logic_ = nullptr;
};

}  // namespace json_test
