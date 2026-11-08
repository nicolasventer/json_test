#pragma once

#include <algorithm>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace json_test
{

	template <typename... Args> class ModuleExpect
	{
	public:
		struct Sent
		{
			std::string topic;
			std::variant<Args...> data;
		};

		void resetExpect() { sent_.clear(); }

		const std::vector<Sent>& getSent() const { return sent_; }

		template <class T> void record(std::string topic, T message)
		{
			sent_.push_back(Sent{std::move(topic), std::move(message)});
		}

		template <class T> const T* rFindSend(const char* topic) const
		{
			const auto itEnd = sent_.rend();
			const auto it = std::find_if(sent_.rbegin(),
				itEnd,
				[&](const Sent& message) { return message.topic == topic && std::holds_alternative<T>(message.data); });
			return it != itEnd ? &std::get<T>(it->data) : nullptr;
		}

	private:
		std::vector<Sent> sent_;
	};

} // namespace json_test
