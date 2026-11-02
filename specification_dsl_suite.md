# Spécification du DSL de test JSON

## 1. Vue d'ensemble

Cette spécification définit un langage spécifique au domaine (*Domain-Specific Language*, DSL) basé sur JSON pour tester un système, simuler des échanges de données, faire avancer le temps virtuel et vérifier son comportement.

Un fichier décrit une suite de tests. Chaque test contient une liste ordonnée d'étapes, représentées par des tableaux JSON dont le premier élément identifie l'opération.

```json
{
  "name": "Suite de démarrage et d'arrêt",
  "b_random": true,
  "tests": [
    {
      "name": "Démarrage du dispositif",
      "expect_timeout_ms": 500,
      "steps": [
        ["reset_expect"],
        ["receive", "cmd", { "value": "START" }],
        ["elapse_ms", 100],
        ["expect_send", "state", { "status": "RUNNING" }]
      ]
    },
    {
      "name": "Absence de message d'erreur",
      "skip": "En attente de la prise en charge du scénario",
      "steps": [
        ["reset_expect"],
        ["receive", "cmd", { "value": "START" }],
        ["elapse_ms", 100],
        ["expect_not_send", "error"]
      ]
    }
  ]
}
```

## 2. Structure de la suite et des tests

### 2.1 Suite de tests

La racine du document est une suite de tests contenant les champs suivants :

| Champ | Type | Obligatoire | Description |
| --- | --- | --- | --- |
| `name` | chaîne | Oui | Nom de la suite de tests. |
| `tests` | tableau | Oui | Liste des tests de la suite. |
| `b_random` | booléen | Oui | Si `true`, exécute les tests dans un ordre aléatoire afin de vérifier qu'ils sont indépendants de leur ordre d'exécution. Si `false`, conserve leur ordre dans le fichier. |

`b_random` doit toujours être fourni explicitement.

### 2.2 Champs d'un test

Chaque élément de `tests` est un objet comportant les champs suivants :

| Champ | Type | Obligatoire | Description |
| --- | --- | --- | --- |
| `name` | chaîne | Oui | Nom du test. |
| `steps` | tableau | Oui | Liste ordonnée des étapes du test. |
| `expect_timeout_ms` | entier positif | Non | Timeout en millisecondes utilisé par `expect_send` et `expect_not_send`. Valeur par défaut : `500`. |
| `skip` | booléen ou chaîne | Non | `true` ignore le test ; une chaîne ignore le test et en indique la raison ; `false` ou l'absence du champ permet son exécution. |

Le champ `expect_timeout_ms` s'applique aux vérifications du test concerné. Il ne modifie pas le temps virtuel contrôlé par `elapse_ms`.

## 3. Liste des mots-clés

| Mot-clé | Syntaxe | Description |
| --- | --- | --- |
| `receive` | `["receive", topic, data]` | Simule la réception de données par le système. |
| `expect_send` | `["expect_send", topic, expected]` | Vérifie les données envoyées par le système pour le topic spécifié. |
| `expect_not_send` | `["expect_not_send", topic]` | Vérifie qu'aucune donnée du topic spécifié n'a été envoyée depuis le dernier `reset_expect`. |
| `elapse_ms` | `["elapse_ms", milliseconds]` | Fait avancer le temps virtuel du nombre de millisecondes spécifié. |
| `reset_expect` | `["reset_expect"]` | Définit un nouveau point de référence pour les vérifications négatives des envois. Ne réinitialise pas l'état du système. |

## 4. Données et simulation des échanges

Les données sont spécifiées directement à l'aide des types de valeurs JSON : objets, tableaux, chaînes de caractères, nombres, booléens et `null`.

### 4.1 `receive`

Syntaxe :

```json
["receive", topic, data]
```

Simule la réception de données par le système. L'argument `data` est une valeur JSON littérale.

Exemple :

```json
["receive", "sensor", { "temperature": 25 }]
```

## 5. Vérifications

### 5.1 Comparaison partielle

Les vérifications qui comparent des données utilisent une comparaison partielle :

- Chaque champ spécifié doit correspondre à la valeur attendue.
- Les champs supplémentaires présents dans les données réelles sont autorisés.
- Les objets imbriqués suivent la même règle de comparaison partielle.
- Par défaut, les valeurs scalaires sont comparées par égalité.
- Les types JSON doivent correspondre.

Par exemple, `{ "status": "READY" }` correspond à `{ "status": "READY", "temperature": 25 }`.

### 5.2 Opérateurs de comparaison

Par défaut, une valeur scalaire est comparée par égalité. Une paire opérateur-valeur peut être utilisée pour une autre comparaison.

| Opérateur | Signification |
| --- | --- |
| `=` | Égal à |
| `!=` | Différent de |
| `>` | Supérieur à |
| `>=` | Supérieur ou égal à |
| `<` | Inférieur à |
| `<=` | Inférieur ou égal à |

Exemple :

```json
["expect_send", "sensor", { "temperature": [">", 20] }]
```

L'opérateur est appliqué au champ correspondant.

### 5.3 `expect_send`

Syntaxe :

```json
["expect_send", topic, expected]
```

Vérifie que le système a envoyé, sur le topic spécifié, un message correspondant aux données attendues. Cette vérification utilise un timeout configurable par `expect_timeout_ms` (500 ms par défaut) afin de laisser au code testé le temps de s'exécuter avant de conclure à l'absence ou à la présence du message.

Ce timeout est un mécanisme de synchronisation en temps réel ; il n'avance pas le temps virtuel du test. Comme le comportement testé dépend du temps virtuel piloté par `elapse_ms`, ce timeout ne doit pas modifier le résultat fonctionnel du test.

Exemple :

```json
["expect_send", "response", { "status": "OK" }]
```

La vérification échoue si aucun message correspondant n'est observé avant l'expiration du timeout, ou si les données ne correspondent pas aux attentes.

### 5.4 `expect_not_send`

Syntaxe :

```json
["expect_not_send", topic]
```

Vérifie qu'aucun message du topic spécifié n'a été envoyé depuis le dernier `reset_expect`, en observant les envois pendant le timeout défini par `expect_timeout_ms` (500 ms par défaut). Ce délai laisse au code testé le temps de s'exécuter ; il ne fait pas avancer le temps virtuel et ne doit pas influencer le résultat fonctionnel du test.

Exemple :

```json
[
  ["reset_expect"],
  ["elapse_ms", 100],
  ["expect_not_send", "error"]
]
```

## 6. Temps virtuel

### 6.1 `elapse_ms`

Syntaxe :

```json
["elapse_ms", milliseconds]
```

Fait avancer l'horloge virtuelle du nombre de millisecondes indiqué et traite les événements programmés pendant cet intervalle, sans attendre en temps réel.

Exemple :

```json
[
  ["receive", "cmd", { "value": "START" }],
  ["elapse_ms", 100],
  ["expect_send", "state", { "status": "RUNNING" }]
]
```

Si une réponse dépend d'un délai virtuel, `elapse_ms` doit être exécuté avant la vérification.

## 7. Point de référence des vérifications

### 7.1 `reset_expect`

Syntaxe :

```json
["reset_expect"]
```

Définit un nouveau point de référence pour `expect_not_send`. Cette opération :

- Ne réinitialise pas l'état du système.
- N'efface pas l'historique des messages.
- Ne fait pas avancer le temps virtuel et ne le remet pas à zéro.

Exemple :

```json
{
  "name": "Vérification de l'arrêt",
  "steps": [
    ["receive", "state", { "status": "RUNNING" }],
    ["elapse_ms", 100],
    ["reset_expect"],
    ["receive", "cmd", { "value": "STOP" }],
    ["elapse_ms", 100],
    ["expect_not_send", "error"]
  ]
}
```

La vérification négative ignore les messages envoyés avant le dernier `reset_expect`.

## 8. Règles d'exécution

1. Les étapes d'un test sont exécutées dans l'ordre où elles apparaissent.
2. Chaque étape se termine avant le début de la suivante.
3. Une vérification qui échoue interrompt le test.
4. `receive` simule la réception de données par le système.
5. `elapse_ms` fait avancer le temps virtuel et traite les événements de manière déterministe.
6. `expect_send` attend au maximum le délai `expect_timeout_ms` pour observer un message correspondant.
7. `expect_not_send` vérifie l'absence de message pendant ce même délai.
8. Les vérifications négatives ne prennent en compte que les messages envoyés depuis le dernier `reset_expect`.
9. Si `skip` vaut `true` ou contient une raison sous forme de chaîne, le test est ignoré.
10. Si `b_random` vaut `true`, l'ordre des tests de la suite est randomisé ; les tests doivent donc être conçus pour réussir indépendamment de cet ordre.
11. Le moteur de test indique le test et l'étape en échec, les données attendues et réelles lorsqu'elles sont disponibles, ainsi que la raison de l'échec.

## 9. Exemple complet

```json
{
  "name": "Suite de démarrage et d'arrêt",
  "b_random": true,
  "tests": [
    {
      "name": "Démarrage du dispositif",
      "expect_timeout_ms": 500,
      "steps": [
        ["reset_expect"],
        ["receive", "cmd", { "value": "START" }],
        ["elapse_ms", 100],
        ["expect_send", "response", { "status": "OK" }]
      ]
    },
    {
      "name": "Arrêt du dispositif",
      "skip": false,
      "steps": [
        ["reset_expect"],
        ["receive", "cmd", { "value": "STOP" }],
        ["elapse_ms", 100],
        ["expect_not_send", "error"]
      ]
    }
  ]
}
```

## 10. Notes d'implémentation

Le moteur doit valider la structure JSON avant l'exécution et rejeter les champs obligatoires absents, les mots-clés inconnus, les nombres d'arguments invalides, les opérateurs de comparaison invalides et les types de valeurs incompatibles.

Le timeout des vérifications est un délai de synchronisation en temps réel, distinct du temps virtuel. Le moteur doit traiter les événements à temps virtuel égal de façon déterministe. Les tests doivent être indépendants les uns des autres, notamment lorsque `b_random` est activé.
