#pragma once

#include <map>
#include <string>
#include <variant>

template <typename... Args>
class VariantMap : public std::map<std::string, std::variant<Args...>>
{
public:
	template <class T> T* find_as(const std::string& key)
	{
		const auto it = this->find(key);
		return it == this->end() ? nullptr : std::get_if<T>(&it->second);
	}

	template <class T> const T* find_as(const std::string& key) const
	{
		const auto it = this->find(key);
		return it == this->end() ? nullptr : std::get_if<T>(&it->second);
	}
};
