# AsyncCall

[English](README.md)

## Sonder un appel bloquant

[`example.cpp`](example.cpp) :

```cpp
#include "async_call.hpp"

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main()
{
	std::cout << "what is your name?\n";

	AsyncCall name([] {
		std::string text;
		std::getline(std::cin, text);
		return text;
	});

	while (true)
	{
		// returns at once: the name, or nothing while cin is still waiting
		if (auto text = name.try_get())
		{
			// name received; say hello and do not read another line
			std::cout << "Hello " << *text << '\n';
			break;
		}
		std::cout << "still waiting your name...\n";
		std::this_thread::sleep_for(std::chrono::seconds{3});
	}
}
```

```text
what is your name?
still waiting your name...
Hello ada
```

L'écho du terminal n'est pas dans cette sortie. Une attente plus longue ajoute une ligne `still waiting your name...` toutes les 3 secondes.

`AsyncCall<F>` garde un thread lancé au constructeur. Ce thread attend, et n'appelle `F` qu'après un `try_get()`. L'appelant sonde le résultat et peut faire autre chose entre deux sondages. Dans l'exemple, le programme écrit `what is your name?`, puis `name` attend une ligne sur `std::cin`. Sans nom, la boucle écrit `still waiting your name...`, puis dort 3 secondes. Avec le nom, elle écrit `Hello` suivi du nom.

`F` est appelable sans argument et renvoie une valeur. Ce retour est stocké dans un `std::optional` : `void` ne compile pas.

Copie et déplacement sont interdits.

## try_get

`try_get()` revient tout de suite.

- Le premier appel arme `F` et renvoie `std::nullopt`. Le thread ne commence le travail qu'une fois le mutex relâché, donc cet appel ne voit jamais le résultat.
- Tant que l'appel est en cours, `try_get()` renvoie `std::nullopt`.
- Quand le résultat est prêt, `try_get()` le consomme et le renvoie. Cet appel ne relance pas `F`. Le `try_get()` suivant le fait.

Le premier `try_get()` de `name` arme l'appel et ne voit pas le nom ; après le sommeil de 3 secondes, le sondage qui obtient le nom ne relance pas `F`, et le `break` arrête la boucle, donc aussi les messages d'attente.

Entre la valeur consommée et le sondage suivant, le thread attend. Rien ne tourne.

## Exceptions

Une exception levée par `F` est capturée dans le thread de travail. Le `try_get()` qui consomme ce résultat la propage sur le thread appelant. Cet appel ne relance pas `F`. Le `try_get()` suivant le fait.

Entre cette exception et le sondage suivant, le thread attend. Rien ne tourne.

## Destruction

Le destructeur demande l'arrêt, réveille le thread et le joint. Si `F` est en cours, il attend qu'elle revienne. Un appel bloquant, comme `std::getline` sur `std::cin`, n'est pas annulé.

Dans l'exemple, le nom est consommé avant la fin de `main`. Le thread attend alors l'arrêt, et le destructeur revient sans lire une ligne de plus. Détruire `AsyncCall` pendant une lecture encore en cours bloque jusqu'au retour de `F`.
