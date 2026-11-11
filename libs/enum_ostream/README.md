# ENUM_OSTREAM

[Français](README_fr.md)

## Printed names

[`example.cpp`](example.cpp):

```cpp
#include "enum_ostream.hpp"

#include <iostream>

enum EMode
{
	IDLE,
	BUSY
};

ENUM_OSTREAM(EMode, IDLE, BUSY)

enum class EStateStatus
{
	INIT,
	RUNNING,
	CLOSED
};

ENUM_OSTREAM(EStateStatus, EStateStatus::INIT, EStateStatus::RUNNING, EStateStatus::CLOSED)

enum class EAlarmLevel
{
	INFO,
	WARN,
	FAULT
};

ENUM_CLASS_OSTREAM(EAlarmLevel, INFO, WARN, FAULT)

int main()
{
	std::cout << IDLE << '\n';
	std::cout << BUSY << '\n';
	std::cout << EStateStatus::INIT << '\n';
	std::cout << EStateStatus::RUNNING << '\n';
	std::cout << EStateStatus::CLOSED << '\n';
	std::cout << EAlarmLevel::INFO << '\n';
	std::cout << EAlarmLevel::WARN << '\n';
	std::cout << EAlarmLevel::FAULT << '\n';
	std::cout << static_cast<EAlarmLevel>(99) << "end\n";
}
```

```text
IDLE
BUSY
EStateStatus::INIT
EStateStatus::RUNNING
EStateStatus::CLOSED
INFO
WARN
FAULT
end
```

Each argument is displayed as written. It also selects the value: as written for `ENUM_OSTREAM`, prefixed with the enum name for `ENUM_CLASS_OSTREAM`.

For an `enum`, either macro works. `ENUM_OSTREAM` leaves the prefix optional: `IDLE` displays `IDLE`, `EMode::IDLE` displays `EMode::IDLE`.

For an `enum class`, either macro works, but the prefix is no longer optional. `ENUM_OSTREAM` expects `EStateStatus::INIT` and displays it. `ENUM_CLASS_OSTREAM` expects `INFO` and displays `INFO`.

Place the macro just after the enum, in the same namespace. From 1 to 8 names; the order does not matter. A value absent from the list writes nothing.
