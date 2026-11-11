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
