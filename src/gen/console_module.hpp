#pragma once

#include "IModule.hpp"
#include "Timer.hpp"

class ConsoleModule : public IModule {
 public:
  explicit ConsoleModule(IModuleLogic& logic);

  Timer& getTimer() override;
  IModuleLogic& getLogic() override;
  void Start() override;

  void sendDeviceStatusStatus(const StatusStatus& message) override;
  void sendDeviceMeasureStatus(const MeasureStatus& message) override;
  void sendDeviceAlarmStatus(const AlarmStatus& message) override;
  void sendDeviceCanMessageStatus(const CanMessageStatus& message) override;

 private:
  Timer timer_;
  IModuleLogic& logic_;
};
