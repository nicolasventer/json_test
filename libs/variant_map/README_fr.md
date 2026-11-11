# VariantMap

[English](README.md)

## Une valeur par clé

[`example.cpp`](example.cpp) :

```cpp
#include "variant_map.hpp"

#include <iostream>
#include <string>

int main()
{
	VariantMap<std::string, int> values;

	values["text"] = std::string{"one"};
	values["text"] = std::string{"two"};
	values["number"] = 7;

	std::cout << values.size() << '\n';

	const std::string* text = values.find_as<std::string>("text");
	std::cout << *text << '\n';

	const int* number = values.find_as<int>("number");
	std::cout << *number << '\n';

	const int* other = values.find_as<int>("text");
	std::cout << (other ? "found" : "none") << '\n';

	const std::string* again = values.find_as<std::string>("text");
	std::cout << *again << '\n';

	values.clear();
	const std::string* none = values.find_as<std::string>("text");
	std::cout << (none ? "found" : "none") << '\n';

	values["number"] = 3;
	const int* after = values.find_as<int>("number");
	std::cout << *after << '\n';
	std::cout << values.size() << '\n';
	std::cout << values.begin()->first << '\n';
}
```

```text
2
two
7
none
two
none
3
1
number
```

`VariantMap<std::string, int>` est une `std::map<std::string, std::variant<std::string, int>>`. La clé `"text"` va avec `std::string`, la clé `"number"` avec `int`. Les crochets écrivent la valeur. `"two"` remplace `"one"`, donc la map a deux éléments. `7` est lu sur `"number"`. Un `int` demandé sur `"text"` donne `nullptr`. Le second `find_as` revoit `"two"`. Après `clear`, il ne reste que la clé `"number"`.

`VariantMap` s'utilise comme une valeur. Le destructeur de `std::map` n'est pas virtuel.

## operator[]

`operator[]` crée la clé si elle est absente, puis l'affectation remplace la valeur. Une seconde affectation de la même clé remplace la précédente. Le variant prend le type de la valeur affectée.

Sur une clé absente, `operator[]` construit d'abord le variant avec sa première alternative, puis l'affectation le remplace. L'exemple ne se sert pas des crochets pour tester une absence : cela insérerait une valeur.

Dans l'exemple, `values["text"] = std::string{"one"}` puis `values["text"] = std::string{"two"}` laisse `"two"`. `values["number"] = 7` range `7`. Après `clear`, `values["number"] = 3` range `3`.

## find_as

`find_as<T>(const std::string& key)` appelle `find`, puis `std::get_if<T>`. `T` est le type associé à la clé. La surcharge non const renvoie `T*`, la surcharge const renvoie `const T*`. L'appel ne retire rien et n'insère rien.

Une clé absente, ou une clé dont la valeur n'est pas `T`, donne `nullptr`.

Dans l'exemple, `find_as<std::string>("text")` renvoie `"two"`. `find_as<int>("number")` renvoie `7`. `find_as<int>("text")` renvoie `nullptr`, parce que `"text"` porte un `std::string`. Le second `find_as<std::string>("text")` vise encore `"two"`. Après `clear`, `find_as<std::string>("text")` renvoie `nullptr`. `find_as<int>("number")` renvoie ensuite `3`.

## Map

`clear` vide la map. `size` et `begin` sont ceux de `std::map`. Ce README ne reprend pas le reste de l'interface.

Après les trois affectations, `size()` vaut `2`. Après `clear` et `values["number"] = 3`, `size()` vaut `1` et `begin()->first` est `"number"`.

## Durée de vie

Le pointeur renvoyé par `find_as` désigne l'élément dans le nœud. Il devient invalide si une écriture remplace cette clé ou rééquilibre la map, et après `clear`.
