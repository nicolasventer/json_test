# VariantMap

[Français](README_fr.md)

## One value per key

[`example.cpp`](example.cpp):

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

`VariantMap<std::string, int>` is a `std::map<std::string, std::variant<std::string, int>>`. The key `"text"` goes with `std::string`, the key `"number"` with `int`. The brackets write the value. `"two"` replaces `"one"`, so the map has two elements. `7` is read from `"number"`. An `int` asked for on `"text"` gives `nullptr`. The second `find_as` sees `"two"` again. After `clear`, only the key `"number"` remains.

`VariantMap` is used as a value. The `std::map` destructor is not virtual.

## operator[]

`operator[]` creates the key when it is absent, then the assignment replaces the value. A second assignment of the same key replaces the previous one. The variant takes the type of the assigned value.

On a missing key, `operator[]` first builds the variant with its first alternative, then the assignment replaces it. The example does not use the brackets to test an absence: that would insert a value.

In the example, `values["text"] = std::string{"one"}` then `values["text"] = std::string{"two"}` leaves `"two"`. `values["number"] = 7` stores `7`. After `clear`, `values["number"] = 3` stores `3`.

## find_as

`find_as<T>(const std::string& key)` calls `find`, then `std::get_if<T>`. `T` is the type associated with the key. The non-const overload returns `T*`, the const overload returns `const T*`. The call removes nothing and inserts nothing.

A missing key, or a key whose value is not `T`, gives `nullptr`.

In the example, `find_as<std::string>("text")` returns `"two"`. `find_as<int>("number")` returns `7`. `find_as<int>("text")` returns `nullptr`, because `"text"` holds a `std::string`. The second `find_as<std::string>("text")` still aims at `"two"`. After `clear`, `find_as<std::string>("text")` returns `nullptr`. `find_as<int>("number")` then returns `3`.

## Map

`clear` empties the map. `size` and `begin` are those of `std::map`. This README does not cover the rest of the interface.

After the three assignments, `size()` is `2`. After `clear` and `values["number"] = 3`, `size()` is `1` and `begin()->first` is `"number"`.

## Lifetime

The pointer returned by `find_as` refers to the element inside the node. It becomes invalid if a write replaces that key or rebalances the map, and after `clear`.
