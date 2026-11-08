# Squelette

## Ordre d'appel

```cpp
json_test::DeviceLogic logic;
json_test::ConsoleModule module{logic};
module.Start();
```

[`src/main.cpp`](src/main.cpp) construit ainsi l'application. [`tests/example1_test.cpp`](tests/example1_test.cpp) fait la même chose avec `TestModule`, puis appelle `module.Start()` dans `SetUp`.

1. Constructeur de `IModuleLogic`. `DeviceLogic logic` construit d'abord cette base.
2. Constructeur de `IModule`. `ConsoleModule` et `TestModule` en héritent : cette base est construite avant les membres du module.
3. `IModuleLogic::setModule`. Le corps du constructeur du module appelle `logic_.setModule(*this)`, dans [`src/console_module.cpp`](src/console_module.cpp) et [`src/test_module.cpp`](src/test_module.cpp).
4. `IModule::Start`. L'appelant l'invoque une fois l'objet construit. La méthode s'écrit `Start`.

## Fichiers à générer

- [`IModule.hpp`](IModule.hpp)
- [`src/console_module.hpp`](src/console_module.hpp), [`src/console_module.cpp`](src/console_module.cpp)
- [`src/main.cpp`](src/main.cpp)
- [`src/test_module.hpp`](src/test_module.hpp), [`src/test_module.cpp`](src/test_module.cpp)
- [`tests/example1_test.cpp`](tests/example1_test.cpp)

## Fichiers à copier

- [`Timer.hpp`](Timer.hpp), [`Timer.cpp`](Timer.cpp)
- [`src/async_call.hpp`](src/async_call.hpp)
- [`src/module_expect.hpp`](src/module_expect.hpp)
- [`enum_ostream.hpp`](enum_ostream.hpp)

## Fichiers à implémenter

- [`src/device_logic.hpp`](src/device_logic.hpp)
- [`src/device_logic.cpp`](src/device_logic.cpp)
