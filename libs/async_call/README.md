# AsyncCall

[Français](README_fr.md)

## Polling a blocking call

[`example.cpp`](example.cpp):

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

The terminal echo is not in this output. A longer wait adds a `still waiting your name...` line every 3 seconds.

`AsyncCall<F>` keeps a thread started by the constructor. That thread waits, and calls `F` only after a `try_get()`. The caller polls the result and can do other work between polls. In the example, the program writes `what is your name?`, then `name` waits for a line on `std::cin`. Without a name, the loop writes `still waiting your name...`, then sleeps 3 seconds. With the name, it writes `Hello` followed by the name.

`F` is callable with no arguments and returns a value. That return is stored in a `std::optional`: `void` does not compile.

Copy and move are forbidden.

## try_get

`try_get()` returns immediately.

- The first call arms `F` and returns `std::nullopt`. The thread starts the work only after the mutex is released, so this call never sees the result.
- While the call is in progress, `try_get()` returns `std::nullopt`.
- When the result is ready, `try_get()` consumes it and returns it. This call does not start `F` again. The next `try_get()` does.

The first `try_get()` of `name` arms the call and does not see the name. After the 3-second sleep, the poll that gets the name does not start `F` again, and the `break` leaves the loop, so the waiting messages stop too.

Between the consumed value and the next poll, the thread waits. Nothing is running.

## Exceptions

An exception thrown by `F` is captured on the worker thread. The `try_get()` that consumes that result rethrows it on the calling thread. This call does not start `F` again. The next `try_get()` does.

Between that rethrow and the next poll, the thread waits. Nothing is running.

## Destruction

The destructor requests a stop, wakes the thread, and joins it. If `F` is running, it waits until `F` returns. A blocking call, such as `std::getline` on `std::cin`, is not cancelled.

In the example, the name is consumed before `main` returns. The thread then waits for the stop, and the destructor returns without reading another line. Destroying `AsyncCall` while a read is still in progress blocks until `F` returns.

## std::future

`AsyncCall` and `std::future` provide the same features: retrieving the result without blocking, and rethrowing exceptions on the calling thread.

**The custom implementation is required by one major difference: `std::async` creates a new thread on every call.**

That model does not fit this library: many calls are expected, and each one would open a thread.
