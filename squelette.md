# Squelette

## Ordre d'appel

```cpp
DeviceLogic logic;
ConsoleModule module{logic};
module.Start();
```

[`src/gen/main.cpp`](src/gen/main.cpp) construit ainsi l'application. [`src/gen/example1_test.cpp`](src/gen/example1_test.cpp) fait la même chose avec `TestModule`, puis appelle `module.Start()` dans `SetUp`.

1. Constructeur de `IModuleLogic`. `DeviceLogic logic` construit d'abord cette base.
2. Constructeur de `IModule`. `ConsoleModule` et `TestModule` en héritent : cette base est construite avant les membres du module.
3. `IModuleLogic::setModule`. Le corps du constructeur du module appelle `logic_.setModule(*this)`, dans [`src/gen/console_module.cpp`](src/gen/console_module.cpp) et [`src/gen/test_module.cpp`](src/gen/test_module.cpp).
4. `IModule::Start`. L'appelant l'invoque une fois l'objet construit. La méthode s'écrit `Start`.

## Fichiers à générer

- [`src/gen/IModule.hpp`](src/gen/IModule.hpp)
- [`src/gen/console_module.hpp`](src/gen/console_module.hpp), [`src/gen/console_module.cpp`](src/gen/console_module.cpp)
- [`src/gen/main.cpp`](src/gen/main.cpp)
- [`src/gen/test_module.hpp`](src/gen/test_module.hpp), [`src/gen/test_module.cpp`](src/gen/test_module.cpp)
- [`src/gen/example1_test.cpp`](src/gen/example1_test.cpp)

## Fichiers à copier

- [`libs/timer/Timer.hpp`](libs/timer/Timer.hpp), [`libs/timer/Timer.cpp`](libs/timer/Timer.cpp)
- [`libs/async_call/async_call.hpp`](libs/async_call/async_call.hpp)
- [`libs/variant_map/variant_map.hpp`](libs/variant_map/variant_map.hpp)
- [`libs/enum_ostream/enum_ostream.hpp`](libs/enum_ostream/enum_ostream.hpp)

## Fichiers à implémenter

- [`src/device_logic.hpp`](src/device_logic.hpp)
- [`src/device_logic.cpp`](src/device_logic.cpp)
