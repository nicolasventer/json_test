#pragma once

#include "IModule.hpp"
#include "Timer.hpp"

#include <memory>

class DeviceLogic : public IModuleLogic {
 public:
  void setModule(IModule& module) override;
  void onReceiveDeviceCmdCommand(const CmdCommand& message) override;
  void onReceiveDeviceConfigStatus(const ConfigStatus& message) override;
  void onReceiveDeviceCanMessageCommand(const CanMessageCommand& message) override;

 private:
  void onTick(int tick_ms);

  IModule* module_ = nullptr;
  ConfigStatus config_{};
  bool has_config_ = false;
  bool started_ = false;
  int processed_ticks_ = 0;
  std::unique_ptr<Timer::TaskHandle> tick_;
};
