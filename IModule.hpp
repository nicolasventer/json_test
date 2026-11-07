#pragma once

#include <string>

#include "enum_ostream_case.hpp"

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

struct cmd
{
	std::string value;
};

struct config
{
	int interval_ms;
	double threshold;
};

struct state
{
	EStateStatus status;
	int uptime_ms;
};

struct measure
{
	double temperature;
	int humidity;
};

struct alarm
{
	EAlarmLevel level;
	int code;
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

	virtual void send(state message) = 0;
	virtual void send(measure message) = 0;
	virtual void send(alarm message) = 0;
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
	virtual void onReceive(cmd message) = 0;
	virtual void onReceive(config message) = 0;
};
