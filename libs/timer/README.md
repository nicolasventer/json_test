# Timer

[Français](README_fr.md)

## Virtual time

```cpp
Timer timer;
timer.startVirtual();

timer.executeAt(std::chrono::milliseconds{100}, [] {
    // once, at t = 100 ms of virtual time
});

timer.sleep(std::chrono::milliseconds{100});
```

## 100 Hz in real time

```cpp
Timer timer;
timer.startReal();

timer.executeAt(std::chrono::milliseconds{10}, [] {
    // about every 10 ms, on this thread
});

timer.sleep(std::chrono::hours{24}); // blocks here and runs the callback
```

`Timer` advances a time and, during `sleep`, runs the registered callbacks. It owns no thread: until someone calls `sleep`, nothing runs. Callbacks run on the calling thread.

Copy and move are forbidden.

## Start

Without a call to `startVirtual` or `startReal`, the timer stays inactive and `sleep` returns immediately.

- `startVirtual()` resets the time to 0. `sleep` advances that time without waiting.
- `startReal()` records `steady_clock::now()`. `sleep` blocks the calling thread with `std::this_thread::sleep_for`.

`now()` returns the virtual time, or the elapsed duration since `startReal`, in milliseconds.

## sleep

`sleep(duration)` is the only driver.

In virtual time, `virtual_now_` is incremented up to the target, stopping at each deadline. In real time, the thread sleeps in slices until the deadline. At each due deadline, `sleep` runs the callbacks.

A negative duration, or a timer that is still inactive, does nothing.

## Tasks

`executeAt(period, callback)` registers a task. It becomes due at `now() + period`, then at every multiple of its period. A zero or negative period becomes 1 ms.

- The `void()` overload ignores the handle. The `void(const TaskHandle&)` overload receives it: `release()` removes the task, a second call does nothing, and the `TaskHandle` destructor calls `release()`.
- `executeAtEx(period, callback)` registers a `void()` callback and returns `std::unique_ptr<TaskHandle>`. Destroying that pointer, or calling `release()`, removes the callback. Ignoring the return destroys the temporary at the end of the statement, so the callback is removed immediately.

`executeAt` and `executeAtEx` do not call the callback. Only a `sleep` that reaches the deadline does.

```cpp
auto tick = timer.executeAtEx(std::chrono::milliseconds{100}, [] {
    // every 100 ms, as long as tick is alive
});

timer.sleep(std::chrono::milliseconds{500});
tick->release(); // or let tick leave scope
```

`std::this_thread::sleep_for` does not guarantee an exact wake-up. On Windows, the real rate oscillates around the requested period. If a callback lasts longer than its period, or if the clock passes several periods, the late deadlines fire in a burst.

## Destruction

No callback is called. The `Timer` destructor detaches each `TaskHandle` still linked, then destroys its tasks: the handle destructor does not call back into a timer already being destroyed. A `unique_ptr<TaskHandle>` that outlives the `Timer` does nothing.
