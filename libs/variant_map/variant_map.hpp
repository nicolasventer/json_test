#pragma once

#include <map>
#include <string>
#include <variant>

// Map from string keys to one of Args. Inherits std::map, so each key holds one value.
template <typename... Args>
class VariantMap : public std::map<std::string, std::variant<Args...>>
{
public:
	// Returns a pointer to the value stored under key when it holds T.
	// Returns nullptr when the key is missing or holds another type.
	template <class T> T* find_as(const std::string& key)
	{
		const auto it = this->find(key);
		return it == this->end() ? nullptr : std::get_if<T>(&it->second);
	}

	// Returns a pointer to the value stored under key when it holds T.
	// Returns nullptr when the key is missing or holds another type.
	template <class T> const T* find_as(const std::string& key) const
	{
		const auto it = this->find(key);
		return it == this->end() ? nullptr : std::get_if<T>(&it->second);
	}
};
