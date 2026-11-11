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
