#pragma once

#include "IModule.hpp"
#include "Timer.hpp"

#include <memory>

namespace json_test {

class DeviceLogic : public IModuleLogic {
 public:
  void setModule(IModule& module) override;
  void onReceive_cmd(const cmd& message) override;
  void onReceive_config(const config& message) override;

 private:
  void onTick(int tick_ms);

  IModule* module_ = nullptr;
  config config_{};
  bool has_config_ = false;
  bool started_ = false;
  int processed_ticks_ = 0;
  std::unique_ptr<Timer::TaskHandle> tick_;
};

}  // namespace json_test
