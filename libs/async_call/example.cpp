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
