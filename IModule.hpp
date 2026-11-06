#pragma once

#include <ostream>
#include <string>

enum EStateStatus
{
    INIT,
    RUNNING,
    CLOSED
};

inline std::ostream& operator<<(std::ostream& os, EStateStatus value)
{
    switch (value)
    {
        case INIT:
            return os << "INIT";
        case RUNNING:
            return os << "RUNNING";
        case CLOSED:
            return os << "CLOSED";
    }
    return os;
}


enum EAlarmLevel
{
    INFO,
    WARN,
    FAULT
};

inline std::ostream& operator<<(std::ostream& os, EAlarmLevel value)
{
    switch (value)
    {
        case INFO:
            return os << "INFO";
        case WARN:
            return os << "WARN";
        case FAULT:
            return os << "FAULT";
    }
    return os;
}

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

class IModuleLogic;

class IModule
{
public:
    virtual ~IModule() = default;
    virtual void setLogic(IModuleLogic* logic) = 0;
    virtual void Start() = 0;

    virtual void send(state message) = 0;
    virtual void send(measure message) = 0;
    virtual void send(alarm message) = 0;
};

class IModuleLogic
{
public:
    virtual ~IModuleLogic() = default;

    virtual void onReceive(cmd message) = 0;
    virtual void onReceive(config message) = 0;
};
