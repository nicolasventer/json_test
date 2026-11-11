# AsyncCall

[Français](README_fr.md)

## Polling a blocking call

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

`AsyncCall<F>` keeps a thread started by the constructor. That thread waits, and calls `F` only after a `try_get()`. The caller polls the result and can do other work between polls. `json_test_app` uses it to read the keyboard while the timer advances.

`F` is callable with no arguments and returns a value. That return is stored in a `std::optional`: `void` does not compile.

Copy and move are forbidden.

## try_get

`try_get()` returns immediately.

- The first call arms `F` and returns `std::nullopt`. The thread starts the work only after the mutex is released, so this call never sees the result.
- While the call is in progress, `try_get()` returns `std::nullopt`.
- When the result is ready, `try_get()` consumes it and returns it. This call does not start `F` again. The next `try_get()` does.

Between the consumed value and the next poll, the thread waits. Nothing is running.

## Exceptions

An exception thrown by `F` is captured on the worker thread. The `try_get()` that consumes that result rethrows it on the calling thread. The worker thread stays alive: the next poll starts `F` again.

## Destruction

The destructor requests a stop, wakes the thread, and joins it. If `F` is running, it waits until `F` returns. A blocking call, such as `Term::read_event`, is not cancelled.

In `main`, the Escape key is consumed before the loop exits. The thread then waits for the stop, and the destructor returns without reading another key. Destroying `AsyncCall` while a read is still in progress blocks until `F` returns.
