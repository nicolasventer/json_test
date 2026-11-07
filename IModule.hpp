#pragma once

#include <ostream>
#include <string>

#include "enum_ostream_case.hpp"

#define ENUM_OSTREAM_FE_1(a1) ENUM_OSTREAM_CASE(a1)
#define ENUM_OSTREAM_FE_2(a1, a2) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_1(a2)
#define ENUM_OSTREAM_FE_3(a1, a2, a3) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_2(a2, a3)
#define ENUM_OSTREAM_FE_4(a1, a2, a3, a4) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_3(a2, a3, a4)
#define ENUM_OSTREAM_FE_5(a1, a2, a3, a4, a5) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_4(a2, a3, a4, a5)
#define ENUM_OSTREAM_FE_6(a1, a2, a3, a4, a5, a6) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_5(a2, a3, a4, a5, a6)
#define ENUM_OSTREAM_FE_7(a1, a2, a3, a4, a5, a6, a7) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_6(a2, a3, a4, a5, a6, a7)
#define ENUM_OSTREAM_FE_8(a1, a2, a3, a4, a5, a6, a7, a8) ENUM_OSTREAM_CASE(a1) ENUM_OSTREAM_FE_7(a2, a3, a4, a5, a6, a7, a8)

#define ENUM_OSTREAM_PICK(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME
#define ENUM_OSTREAM_EXPAND(x) x

#define ENUM_OSTREAM(Enum, ...) \
    inline std::ostream& operator<<(std::ostream& os, Enum value) \
    { \
        switch (value) \
        { \
            ENUM_OSTREAM_EXPAND(ENUM_OSTREAM_PICK(__VA_ARGS__, ENUM_OSTREAM_FE_8, ENUM_OSTREAM_FE_7, ENUM_OSTREAM_FE_6, ENUM_OSTREAM_FE_5, ENUM_OSTREAM_FE_4, ENUM_OSTREAM_FE_3, ENUM_OSTREAM_FE_2, ENUM_OSTREAM_FE_1)(__VA_ARGS__)) \
        } \
        return os; \
    }

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
