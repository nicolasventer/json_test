# json_test

Projet C++17 qui simule un dispositif et vérifie les messages qu'il émet.

`DeviceLogic` implémente la logique du dispositif derrière `IModule`. À la réception d'une `config` puis d'une commande `START`, chaque tick du minuteur envoie des messages `state`, `measure` ou `alarm`. `TestModule` enregistre ces envois. `ModuleExpect` est le journal consulté par les tests. Son usage est décrit dans [`ModuleExpect.md`](ModuleExpect.md). `Timer` fait avancer un temps virtuel, sans attendre le temps réel. Son usage est décrit dans [`Timer.md`](Timer.md). `AsyncCall` lit le clavier de `json_test_app`. Son usage est décrit dans [`AsyncCall.md`](AsyncCall.md). `ENUM_OSTREAM` écrit le nom d'un énumérateur sur un flux. Son usage est décrit dans [`enum_ostream.md`](enum_ostream.md). L'ordre d'appel et le classement des fichiers sont décrits dans [`squelette.md`](squelette.md).

[`specification_dsl_suite.md`](specification_dsl_suite.md) décrit un DSL de tests au format JSON. [`example1.json`](example1.json) est une suite d'exemple pour ce DSL. Les tests du dépôt sont écrits en C++ avec GoogleTest et rejouent ce scénario : aucun programme ne charge le fichier JSON.

## Prérequis

- CMake 3.16 ou plus récent
- un compilateur C++17

GoogleTest 1.15.2 et [cpp-terminal](https://github.com/jupyter-xeus/cpp-terminal/tree/master) (branche `master`) sont téléchargés par CMake (FetchContent) lors de la première configuration. Cette étape a besoin d'un accès réseau.

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

Le programme a besoin d'un terminal (TTY) et tourne en temps réel. Au démarrage, il affiche les touches :

- `a` envoie la commande `START`
- `b` envoie la config (`interval_ms` 100, `threshold` 30.5)
- Tab réaffiche ces touches
- Échap quitte

Toute autre touche affiche `receive unknown key` suivi du nom de la touche (`c`, `Space`, `Left Arrow`…), et non de son code numérique.

Les messages `state`, `measure` et `alarm` du dispositif s'affichent sur la sortie standard.

## Tester

```powershell
ctest --test-dir build -C Debug --output-on-failure
```

On peut aussi lancer le binaire directement :

```powershell
.\build\tests\Debug\json_test_tests.exe
```

Trois tests de `SuiteDispositif` (`DemarrageDuDispositif`, `MesureSousLeSeuil`, `SeuilDepasse`) doivent passer. `ArretDuDispositif` échoue volontairement : le traitement de la commande `STOP` est laissé inactif dans [`src/device_logic.cpp`](src/device_logic.cpp).
