# ENUM_OSTREAM

[Français](README_fr.md)

## Name on the stream

```cpp
enum EStateStatus
{
    INIT,
    RUNNING,
    CLOSED
};

ENUM_OSTREAM(EStateStatus, INIT, RUNNING, CLOSED)

std::cout << RUNNING << '\n'; // RUNNING
```

`ENUM_OSTREAM(Enum, ...)` defines `operator<<` for that enum. The stream receives the enumerator name, as written in the macro, and not its numeric value.

[`src/gen/IModule.hpp`](../../src/gen/IModule.hpp) uses it for `EStateStatus` and `EAlarmLevel`. [`src/gen/console_module.cpp`](../../src/gen/console_module.cpp) thus prints `state RUNNING 100` and `alarm WARN 42`. GoogleTest also uses it to show these values when an expectation fails.

The macro is placed just after the enum, in the same namespace. The names must be visible there. Here, `INIT`, `RUNNING`, and `CLOSED` are those of the unscoped enum. The header is [`enum_ostream.hpp`](enum_ostream.hpp).

## List

The list holds from 1 to 8 names. Each name is a `switch` branch and the printed text. The order of the list does not change the output.

A value absent from the list writes nothing: the `switch` ends and the stream is returned unchanged. This applies to a forgotten enumerator, or to an integer converted to the enum that matches no case.
