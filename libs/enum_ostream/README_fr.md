# ENUM_OSTREAM

[English](README.md)

## Nom sur le flux

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

`ENUM_OSTREAM(Enum, ...)` définit `operator<<` pour cet enum. Le flux reçoit le nom de l'énumérateur, tel qu'il est écrit dans la macro, et non sa valeur numérique.

[`src/gen/IModule.hpp`](../../src/gen/IModule.hpp) l'utilise pour `EStateStatus` et `EAlarmLevel`. [`src/gen/console_module.cpp`](../../src/gen/console_module.cpp) affiche ainsi `state RUNNING 100` et `alarm WARN 42`. GoogleTest s'en sert aussi pour montrer ces valeurs quand une attente échoue.

La macro se place juste après l'enum, dans le même espace de noms. Les noms doivent y être visibles. Ici, `INIT`, `RUNNING` et `CLOSED` sont ceux de l'enum non porté. L'en-tête est [`enum_ostream.hpp`](enum_ostream.hpp).

## Liste

La liste compte de 1 à 8 noms. Chaque nom est une branche du `switch` et le texte affiché. L'ordre de la liste ne change pas l'affichage.

Une valeur absente de la liste n'écrit rien : le `switch` se termine et le flux est renvoyé tel quel. Cela vaut pour un énumérateur oublié, ou pour un entier converti vers l'enum qui ne correspond à aucun cas.
