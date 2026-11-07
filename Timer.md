# Timer

## Temps virtuel

```cpp
Timer timer;
timer.startVirtual();

timer.executeAt(std::chrono::milliseconds{100}, [] {
    // une fois, à t = 100 ms de temps virtuel
});

timer.sleep(std::chrono::milliseconds{100});
```

## 100 Hz en temps réel

```cpp
Timer timer;
timer.startReal();

timer.executeAt(std::chrono::milliseconds{10}, [] {
    // environ toutes les 10 ms, sur ce thread
});

timer.sleep(std::chrono::hours{24}); // bloque ici et fait tourner la callback
```

`Timer` avance un temps et, pendant `sleep`, exécute les callbacks enregistrées. Il ne possède aucun thread : tant que personne n'appelle `sleep`, rien ne part. Les callbacks s'exécutent sur le thread appelant.

Copie et déplacement sont interdits.

## Démarrage

Sans appel à `startVirtual` ou `startReal`, le minuteur reste inactif et `sleep` revient tout de suite.

- `startVirtual()` remet le temps à 0. `sleep` avance ce temps sans attendre.
- `startReal()` mémorise `steady_clock::now()`. `sleep` bloque le thread appelant avec `std::this_thread::sleep_for`.

`now()` renvoie le temps virtuel, ou la durée écoulée depuis `startReal`, en millisecondes.

## sleep

`sleep(duration)` est le seul moteur.

En temps virtuel, `virtual_now_` est incrémenté jusqu'à la cible, en s'arrêtant à chaque échéance. En temps réel, le thread dort par tranches jusqu'à l'échéance. À chaque échéance due, `sleep` lance les callbacks.

Une durée négative, ou un minuteur encore inactif, ne fait rien.

## Tâches

`executeAt(periode, callback)` enregistre une tâche. Elle devient due à `now() + periode`, puis à chaque multiple de sa période. Une période nulle ou négative devient 1 ms.

- La surcharge `void()` ignore le handle. La surcharge `void(const TaskHandle&)` le reçoit : `release()` retire la tâche, un second appel ne fait rien, et le destructeur de `TaskHandle` appelle `release()`.
- `executeAtEx(periode, callback)` enregistre une callback `void()` et renvoie `std::unique_ptr<TaskHandle>`. Détruire ce pointeur, ou appeler `release()`, retire la callback. Ignorer le retour détruit le temporaire à la fin de l'instruction, donc retire la callback tout de suite.

`executeAt` et `executeAtEx` n'appellent pas la callback. Seul un `sleep` qui atteint l'échéance le fait.

```cpp
auto tick = timer.executeAtEx(std::chrono::milliseconds{100}, [] {
    // toutes les 100 ms, tant que tick est vivant
});

timer.sleep(std::chrono::milliseconds{500});
tick->release(); // ou laisser tick sortir de portée
```

`std::this_thread::sleep_for` ne garantit pas un réveil exact. Sous Windows, la cadence réelle oscille autour de la période demandée. Si une callback dure plus longtemps que sa période, ou si l'horloge dépasse plusieurs périodes, les échéances en retard partent en rafale.

## Destruction

Aucune callback n'est appelée. Le destructeur de `Timer` détache chaque `TaskHandle` encore lié, puis détruit ses tâches : le destructeur du handle ne rappelle pas un minuteur déjà en cours de destruction. Un `unique_ptr<TaskHandle>` qui survivrait au `Timer` ne fait plus rien.
