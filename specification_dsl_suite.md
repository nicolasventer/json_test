# Spécification du DSL de test JSON

## 1. Vue d'ensemble

Cette spécification définit un langage spécifique au domaine (_Domain-Specific Language_, DSL) basé sur JSON pour tester un système, simuler des échanges de données, faire avancer le temps virtuel et vérifier son comportement.

Un fichier décrit une suite de tests. Chaque test contient une liste ordonnée d'étapes, représentées par des tableaux JSON dont le premier élément identifie l'opération.

```json
{
	"name": "Suite de démarrage et d'arrêt",
	"tests": [
		{
			"name": "Démarrage du dispositif",
			"steps": [
				["reset_expect"],
				["receive", "cmd", { "value": "START" }],
				["sleep_ms", 100],
				["expect_send", "state", { "status": "RUNNING" }]
			]
		},
		{
			"name": "Absence de message d'erreur",
			"steps": [["reset_expect"], ["receive", "cmd", { "value": "START" }], ["sleep_ms", 100], ["expect_not_send", "error"]]
		}
	]
}
```

## 2. Structure de la suite et des tests

### 2.1 Suite de tests

La racine du document est une suite de tests contenant les champs suivants :

| Champ   | Type    | Description                  |
| ------- | ------- | ---------------------------- |
| `name`  | chaîne  | Nom de la suite de tests.    |
| `tests` | tableau | Liste des tests de la suite. |

### 2.2 Champs d'un test

Chaque élément de `tests` est un objet comportant les champs suivants :

| Champ   | Type    | Description                        |
| ------- | ------- | ---------------------------------- |
| `name`  | chaîne  | Nom du test.                       |
| `steps` | tableau | Liste ordonnée des étapes du test. |

## 3. Liste des mots-clés

| Mot-clé           | Syntaxe                            | Description                                                                                                               |
| ----------------- | ---------------------------------- | ------------------------------------------------------------------------------------------------------------------------- |
| `receive`         | `["receive", topic, data]`         | Simule la réception de données par le système.                                                                            |
| `expect_send`     | `["expect_send", topic, expected]` | Vérifie immédiatement les données déjà envoyées par le système pour le topic spécifié.                                    |
| `expect_not_send` | `["expect_not_send", topic]`       | Vérifie immédiatement qu'aucune donnée du topic spécifié n'a été envoyée depuis le dernier `reset_expect`.                |
| `sleep_ms`        | `["sleep_ms", milliseconds]`       | Fait avancer le temps virtuel du nombre de millisecondes spécifié.                                                        |
| `reset_expect`    | `["reset_expect"]`                 | Définit un nouveau point de référence pour les vérifications négatives des envois. Ne réinitialise pas l'état du système. |

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

Par défaut, une valeur scalaire est comparée par égalité. Une paire opérateur-valeur peut être utilisée pour une autre comparaison. L'opérateur `~=` prend un second paramètre, le seuil.

| Opérateur | Signification            |
| --------- | ------------------------ |
| `=`       | Égal à                   |
| `!=`      | Différent de             |
| `>`       | Supérieur à              |
| `>=`      | Supérieur ou égal à      |
| `<`       | Inférieur à              |
| `<=`      | Inférieur ou égal à      |
| `~=`      | Approximativement égal à |

Exemple :

```json
["expect_send", "sensor", { "temperature": [">", 20] }]
```

```json
["expect_send", "sensor", { "temperature": ["~=", 25, 0.5] }]
```

L'opérateur est appliqué au champ correspondant. Avec `~=`, la valeur observée est approximativement égale à la valeur attendue lorsque la différence absolue est inférieure ou égale au seuil. Le seuil est un nombre positif ou nul. Dans l'exemple, la température doit être comprise entre 24,5 et 25,5.

### 5.3 `expect_send`

Syntaxe :

```json
["expect_send", topic, expected]
```

Vérifie immédiatement que le système a déjà envoyé, sur le topic spécifié, un message correspondant aux données attendues. Cette vérification ne patiente pas et ne fait pas avancer le temps virtuel. Lorsqu'un délai virtuel est nécessaire, `sleep_ms` est placé avant la vérification.

Exemple :

```json
["expect_send", "response", { "status": "OK" }]
```

La vérification échoue si aucun message correspondant n'a été envoyé, ou si les données ne correspondent pas aux attentes.

### 5.4 `expect_not_send`

Syntaxe :

```json
["expect_not_send", topic]
```

Vérifie immédiatement qu'aucun message du topic spécifié n'a été envoyé depuis le dernier `reset_expect`. Cette vérification ne patiente pas et ne fait pas avancer le temps virtuel.

Exemple :

```json
[["reset_expect"], ["sleep_ms", 100], ["expect_not_send", "error"]]
```

## 6. Temps virtuel

### 6.1 `sleep_ms`

Syntaxe :

```json
["sleep_ms", milliseconds]
```

Fait avancer l'horloge virtuelle du nombre de millisecondes indiqué et traite les événements programmés pendant cet intervalle, sans attendre en temps réel. Les événements programmés au même instant virtuel sont traités de façon déterministe.

Exemple :

```json
[
	["receive", "cmd", { "value": "START" }],
	["sleep_ms", 100],
	["expect_send", "state", { "status": "RUNNING" }]
]
```

Si une réponse dépend d'un délai virtuel, `sleep_ms` doit être exécuté avant la vérification.

## 7. Point de référence des vérifications

### 7.1 `reset_expect`

Syntaxe :

```json
["reset_expect"]
```

Définit un nouveau point de référence pour `expect_not_send`. Cette opération :

- Ne réinitialise pas l'état du système.
- Efface les envois enregistrés.
- Ne fait pas avancer le temps virtuel et ne le remet pas à zéro.

Exemple :

```json
{
	"name": "Vérification de l'arrêt",
	"steps": [
		["receive", "state", { "status": "RUNNING" }],
		["sleep_ms", 100],
		["reset_expect"],
		["receive", "cmd", { "value": "STOP" }],
		["sleep_ms", 100],
		["expect_not_send", "error"]
	]
}
```

La vérification négative ignore les messages envoyés avant le dernier `reset_expect`.

## 8. Règles d'exécution

Les étapes d'un test s'exécutent selon les règles suivantes :

1. Les étapes d'un test s'exécutent dans l'ordre où elles apparaissent.
2. Chaque étape se termine avant le début de la suivante.
3. Une vérification qui échoue interrompt le test.
4. `receive` simule la réception de données par le système.
5. `sleep_ms` fait avancer le temps virtuel et traite les événements de manière déterministe.
6. `expect_send` vérifie immédiatement qu'un message correspondant a déjà été envoyé.
7. `expect_not_send` vérifie immédiatement l'absence de message du topic depuis le dernier `reset_expect`.
8. En cas d'échec, le test indique le test et l'étape en échec, les données attendues et réelles lorsqu'elles sont disponibles, ainsi que la raison de l'échec.

## 9. Exemple complet

```json
{
	"name": "Suite de démarrage et d'arrêt",
	"tests": [
		{
			"name": "Démarrage du dispositif",
			"steps": [
				["reset_expect"],
				["receive", "cmd", { "value": "START" }],
				["sleep_ms", 100],
				["expect_send", "response", { "status": "OK" }]
			]
		},
		{
			"name": "Arrêt du dispositif",
			"steps": [["reset_expect"], ["receive", "cmd", { "value": "STOP" }], ["sleep_ms", 100], ["expect_not_send", "error"]]
		}
	]
}
```

## 10. Règles d'écriture

Une suite est invalide lorsqu'un champ est absent, qu'un mot-clé est inconnu, que le nombre d'arguments d'une étape est incorrect, qu'un opérateur de comparaison est invalide ou qu'un type de valeur est incompatible.
