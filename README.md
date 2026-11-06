# json_test

Projet C++17 qui simule un dispositif et vérifie les messages qu'il émet.

`DeviceLogic` implémente la logique du dispositif derrière `IModule`. À la réception d'une `config` puis d'une commande `START`, chaque tick du minuteur envoie des messages `state`, `measure` ou `alarm`. `TestModule` enregistre ces envois. `Timer` fait avancer un temps virtuel, sans attendre le temps réel. Son usage est décrit dans [`Timer.md`](Timer.md).

[`specification_dsl_suite.md`](specification_dsl_suite.md) décrit un DSL de tests au format JSON. [`example1.json`](example1.json) est une suite d'exemple pour ce DSL. Les tests du dépôt sont écrits en C++ avec GoogleTest et rejouent ce scénario : aucun programme ne charge le fichier JSON.

## Prérequis

- CMake 3.16 ou plus récent
- un compilateur C++17

GoogleTest 1.15.2 est téléchargé par CMake (FetchContent) lors de la première configuration. Cette étape a besoin d'un accès réseau.

## Compiler

Depuis la racine du dépôt, avec le générateur Visual Studio :

```powershell
cmake -S . -B build
cmake --build build --config Debug
```

Cela produit la bibliothèque `json_test`, l'exécutable `json_test_app` et le binaire de tests `json_test_tests`.

## Lancer

```powershell
.\build\Debug\json_test_app.exe
```

Le programme affiche `3`, le résultat de `json_test::add(1, 2)`.

## Tester

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

On peut aussi lancer le binaire directement :

```powershell
.\build\tests\Debug\json_test_tests.exe
```

`AddTest` et trois tests de `SuiteDispositif` (`DemarrageDuDispositif`, `MesureSousLeSeuil`, `SeuilDepasse`) doivent passer. `ArretDuDispositif` échoue volontairement : le traitement de la commande `STOP` est laissé inactif dans [`src/device_logic.cpp`](src/device_logic.cpp).
