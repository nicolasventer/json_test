# ENUM_OSTREAM

[English](README.md)

## Noms affichés

[`example.cpp`](example.cpp) :

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

Chaque argument est affiché tel qu'il est écrit. Il sert aussi à retrouver la valeur : tel quel pour `ENUM_OSTREAM`, précédé du nom de l'enum pour `ENUM_CLASS_OSTREAM`.

Pour un `enum`, les deux macros conviennent. `ENUM_OSTREAM` laisse le préfixe au choix : `IDLE` affiche `IDLE`, `EMode::IDLE` affiche `EMode::IDLE`.

Pour un `enum class`, les deux macros conviennent, mais le préfixe n'est plus au choix. `ENUM_OSTREAM` attend `EStateStatus::INIT` et l'affiche. `ENUM_CLASS_OSTREAM` attend `INFO` et affiche `INFO`.

La macro se place juste après l'enum, dans le même espace de noms. De 1 à 8 noms ; l'ordre ne change rien. Une valeur absente de la liste n'écrit rien.
