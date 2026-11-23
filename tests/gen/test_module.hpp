#pragma once

#include "IModule.hpp"
#include "Timer.hpp"
#include "variant_map.hpp"

class TestModule : public IModule {
 public:
  VariantMap<StatusStatus, MeasureStatus, AlarmStatus, CanMessageStatus> expect;

  TestModule(IModuleLogic& logic, bool useVirtualTime);

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
  bool use_virtual_time_;
};
