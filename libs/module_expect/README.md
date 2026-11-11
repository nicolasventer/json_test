# ModuleExpect

[Français](README_fr.md)

## Send log

```cpp
module.expect.resetExpect();

logic.onReceive_config(config{100, 30.5});
logic.onReceive_cmd(cmd{"START"});
module.getTimer().sleep(std::chrono::milliseconds{100});

const state* sent = module.expect.rFindSend<state>("state");
// sent->status, sent->uptime_ms

module.expect.resetExpect();
const alarm* none = module.expect.rFindSend<alarm>("alarm"); // nullptr
```

`ModuleExpect<Args...>` keeps, in order, the sends `{topic, std::variant<Args...>}`. `TestModule` calls `record` from `send_state`, `send_measure`, and `send_alarm`. The tests read this log.

## record

`record(topic, message)` appends a send. The message type must be one of the variant alternatives.

## rFindSend

`rFindSend<T>(topic)` walks the log backwards. It returns a pointer to the most recent message whose topic is `topic` and whose type is `T`, or `nullptr`. A send on the same topic carried by another type is ignored.

It removes nothing. Two calls with no `record` between them target the same message. `MesureSousLeSeuil` reads the last `measure` twice this way.

## resetExpect

`resetExpect()` clears the whole log. It does not change the device state. A later `rFindSend` only sees the sends that follow. `DemarrageDuDispositif` uses it to check that no `alarm` followed the `state`.

## getSent

`getSent()` returns the vector of sends since construction, or since the last `resetExpect`. Each element is a `Sent`: `topic` and `data`.

## Lifetime

The pointer returned by `rFindSend` refers to an element of the vector. That pointer, and any reference to an element of `getSent()`, become invalid if `record` reallocates the vector, and after `resetExpect`.
