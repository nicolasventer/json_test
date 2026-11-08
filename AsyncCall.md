# AsyncCall

## Sonder un appel bloquant

```cpp
AsyncCall keys([] {
    while (true) {
        auto event = Term::read_event();
        if (event.type() == Term::Event::Type::Key) {
            return Term::Key(event);
        }
    }
});

while (true) {
    if (auto key = keys.try_get()) {
        if (*key == Term::Key::Esc) {
            break;
        }
    } else {
        timer.sleep(std::chrono::milliseconds{20});
    }
}
```

`AsyncCall<F>` garde un thread lancé au constructeur. Ce thread attend, et n'appelle `F` qu'après un `try_get()`. L'appelant sonde le résultat et peut faire autre chose entre deux sondages. `json_test_app` s'en sert pour lire le clavier pendant que le minuteur avance.

`F` est appelable sans argument et renvoie une valeur. Ce retour est stocké dans un `std::optional` : `void` ne compile pas.

Copie et déplacement sont interdits.

## try_get

`try_get()` revient tout de suite.

- Le premier appel arme `F` et renvoie `std::nullopt`. Le thread ne commence le travail qu'une fois le mutex relâché, donc cet appel ne voit jamais le résultat.
- Tant que l'appel est en cours, `try_get()` renvoie `std::nullopt`.
- Quand le résultat est prêt, `try_get()` le consomme et le renvoie. Cet appel ne relance pas `F`. Le `try_get()` suivant le fait.

Entre la valeur consommée et le sondage suivant, le thread attend. Rien ne tourne.

## Exceptions

Une exception levée par `F` est capturée dans le thread de travail. Le `try_get()` qui consomme ce résultat la relance sur le thread appelant. Le thread de travail reste vivant : le sondage suivant relance `F`.

## Destruction

Le destructeur demande l'arrêt, réveille le thread et le joint. Si `F` est en cours, il attend qu'elle revienne. Un appel bloquant, comme `Term::read_event`, n'est pas annulé.

Dans `main`, la touche Échap est consommée avant la sortie de la boucle. Le thread attend alors l'arrêt, et le destructeur revient sans lire une touche de plus. Détruire `AsyncCall` pendant une lecture encore en cours bloque jusqu'au retour de `F`.
