#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "enum_ostream.hpp"

enum EStateStatus
{
	INIT,
	RUNNING,
	CLOSED
};

ENUM_OSTREAM(EStateStatus, INIT, RUNNING, CLOSED)

enum EAlarmLevel
{
	INFO,
	WARN,
	FAULT
};

ENUM_OSTREAM(EAlarmLevel, INFO, WARN, FAULT)

enum EProbeKind
{
	TEMP,
	HUMID
};

ENUM_OSTREAM(EProbeKind, TEMP, HUMID)

struct Probe
{
	EProbeKind kind;
};

struct Limit
{
	EAlarmLevel level;
	double value;
};

struct CmdCommand
{
	std::string value;
};

struct ConfigStatus
{
	int interval_ms;
	double threshold;
	Probe probe;
	std::vector<Limit> limits;
};

struct StatusStatus
{
	EStateStatus status;
	int uptime_ms;
};

struct MeasureStatus
{
	double temperature;
	int humidity;
	std::vector<Limit> samples;
};

struct AlarmStatus
{
	EAlarmLevel level;
	int code;
};

struct CanMessageCommand
{
	uint32_t id_code;
	uint64_t data;
};

struct CanMessageStatus
{
	uint32_t id_code;
	uint64_t data;
};

class Timer;
class IModuleLogic;

class IModule
{
public:
	IModule() = default;
	virtual ~IModule() = default;

	IModule(const IModule&) = delete;
	IModule& operator=(const IModule&) = delete;
	IModule(IModule&&) = delete;
	IModule& operator=(IModule&&) = delete;

	virtual Timer& getTimer() = 0;
	virtual IModuleLogic& getLogic() = 0;
	virtual void Start() = 0;

	virtual void sendDeviceStatusStatus(const StatusStatus& message) = 0;
	virtual void sendDeviceMeasureStatus(const MeasureStatus& message) = 0;
	virtual void sendDeviceAlarmStatus(const AlarmStatus& message) = 0;
	virtual void sendDeviceCanMessageStatus(const CanMessageStatus& message) = 0;
};

class IModuleLogic
{
public:
	IModuleLogic() = default;
	virtual ~IModuleLogic() = default;

	IModuleLogic(const IModuleLogic&) = delete;
	IModuleLogic& operator=(const IModuleLogic&) = delete;
	IModuleLogic(IModuleLogic&&) = delete;
	IModuleLogic& operator=(IModuleLogic&&) = delete;

	virtual void setModule(IModule& module) = 0;
	virtual void onReceiveDeviceCmdCommand(const CmdCommand& message) = 0;
	virtual void onReceiveDeviceConfigStatus(const ConfigStatus& message) = 0;
	virtual void onReceiveDeviceCanMessageCommand(const CanMessageCommand& message) = 0;
};
