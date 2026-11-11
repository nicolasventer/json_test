#pragma once

#include "IModule.hpp"
#include "Timer.hpp"

class ConsoleModule : public IModule {
 public:
  explicit ConsoleModule(IModuleLogic& logic);

  Timer& getTimer() override;
  IModuleLogic& getLogic() override;
  void Start() override;

  void send_state(const state& message) override;
  void send_measure(const measure& message) override;
  void send_alarm(const alarm& message) override;

 private:
  Timer timer_;
  IModuleLogic& logic_;
};
