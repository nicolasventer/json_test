# ModuleExpect

[English](README.md)

## Journal des envois

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

`ModuleExpect<Args...>` conserve, dans l'ordre, les envois `{topic, std::variant<Args...>}`. `TestModule` appelle `record` depuis `send_state`, `send_measure` et `send_alarm`. Les tests lisent ce journal.

## record

`record(topic, message)` ajoute un envoi à la fin. Le type du message doit être l'une des alternatives du variant.

## rFindSend

`rFindSend<T>(topic)` parcourt le journal à l'envers. Il renvoie un pointeur vers le message le plus récent dont le topic est `topic` et le type est `T`, ou `nullptr`. Un envoi du même topic porté par un autre type est ignoré.

Il ne retire rien. Deux appels sans `record` entre les deux visent le même message. `MesureSousLeSeuil` lit ainsi deux fois le dernier `measure`.

## resetExpect

`resetExpect()` vide tout le journal. Il ne change pas l'état du dispositif. Un `rFindSend` ensuite ne voit que les envois postérieurs. `DemarrageDuDispositif` s'en sert pour vérifier qu'aucun `alarm` n'a suivi le `state`.

## getSent

`getSent()` renvoie le vecteur des envois depuis la construction, ou depuis le dernier `resetExpect`. Chaque élément est un `Sent` : `topic` et `data`.

## Durée de vie

Le pointeur renvoyé par `rFindSend` désigne un élément du vecteur. Ce pointeur, et toute référence à un élément de `getSent()`, deviennent invalides si `record` réalloue le vecteur, et après `resetExpect`.
