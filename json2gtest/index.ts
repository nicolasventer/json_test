import fs from "fs";
import path from "path";
import mustache from "mustache";
// @ts-expect-error cannot handle raw
import tpl from "./google_tests.cpp.mustache?raw";

type Operator = "EQ" | "NE" | "GT" | "GE" | "LT" | "LE" | "NEAR";

type Pack = (bits: string, start: number) => bigint;

type Step =
	| ["reset_expect"]
	| ["sleep_ms", number]
	| ["receive", string, any]
	| ["expect_send", string, any]
	| ["expect_not_send", string]
	| ["expect_not_send", string, any];

type JsonTest = {
	name: string;
	steps: Step[];
};

type JsonTestSuite = {
	name: string;
	use_virtual_time: boolean;
	tests: JsonTest[];
};

type TypeInfo = {
	topicList: string[];
	enumFieldMap: Record<string, string>;
	isCan: boolean;
};

type TypeCatalog = Record<string, TypeInfo>;

type IdMap = Record<string, number>;

type FieldView = {
	path: string;
	value: string;
	isEnum: boolean;
	operator: Operator;
	epsilon: string;
	integer_near: boolean;
};

type CanView = {
	topic_with_slash: string;
	id_code_number: string;
	id_code: string;
	data: string;
	id_text: string;
	source_comment: string;
};

type StepView = {
	blank_before: boolean;
	is_reset: boolean;
	is_sleep: boolean;
	is_receive: boolean;
	is_expect_send: boolean;
	is_expect_not_send: boolean;
	is_can: boolean;
	ms: string;
	type_name: string;
	topic_pascal: string;
	var_name: string;
	topic_key: string;
	comment_lines: string[];
	fields: FieldView[];
	can: CanView;
};

type View = {
	suite_class: string;
	use_virtual_time: boolean;
	tests: Array<{
		method: string;
		steps: StepView[];
	}>;
};

const OPERATOR_OF: Record<string, Operator> = {
	"=": "EQ",
	"!=": "NE",
	">": "GT",
	">=": "GE",
	"<": "LT",
	"<=": "LE",
	"~=": "NEAR",
};

// Stops generation.
// The message is shown as given.
function fail(message: string): never {
	throw new Error(message);
}

// Recognizes a JSON object.
// Excludes null and arrays, which are also objects in JavaScript.
function isPlainObject(value: any): value is Record<string, any> {
	return typeof value === "object" && value !== null && !Array.isArray(value);
}

// Recognizes a comparison operator.
// Used to tell a comparator apart from a data array.
function isComparatorOp(value: any): value is string {
	return typeof value === "string" && Object.hasOwn(OPERATOR_OF, value);
}

// Builds the fixture or test name.
// Strips accents, Pascal-cases each word, and does not check the result.
function toCppName(label: string, where: string): string {
	const parts = label
		.normalize("NFD")
		.replace(/\p{M}/gu, "")
		.split(/[^A-Za-z0-9]+/)
		.filter((part) => part.length > 0);
	if (parts.length === 0) fail(`${where} vide`);
	return parts.map((part) => part.charAt(0).toUpperCase() + part.slice(1).toLowerCase()).join("");
}

// Writes a C++ string literal.
// Escapes backslash, quote, newline, carriage return, and tab.
function cppString(value: string, where: string): string {
	let out = '"';
	for (const ch of value) {
		switch (ch) {
			case "\\":
				out += "\\\\";
				break;
			case '"':
				out += '\\"';
				break;
			case "\n":
				out += "\\n";
				break;
			case "\r":
				out += "\\r";
				break;
			case "\t":
				out += "\\t";
				break;
			default:
				if (ch < " ") fail(`${where}: caractère de contrôle dans une chaîne`);
				out += ch;
		}
	}
	return `${out}"`;
}

// Writes the inside of a C++ string literal.
// The surrounding quotes are added by the template.
function cppStringContents(value: string, where: string): string {
	return cppString(value, where).slice(1, -1);
}

// Converts a JSON scalar into a C++ token.
// When enumType is set, a string becomes TypeEnum::Value.
function cppToken(value: any, where: string, enumType: string | undefined): { value: string; isEnum: boolean } {
	if (typeof value === "string") {
		if (enumType !== undefined) return { value: `${enumType}::${value}`, isEnum: true };
		return { value: cppString(value, where), isEnum: false };
	}
	if (typeof value === "number") {
		if (!Number.isFinite(value)) fail(`${where}: nombre invalide`);
		return { value: String(value), isEnum: false };
	}
	if (typeof value === "boolean") return { value: value ? "true" : "false", isEnum: false };
	if (value === null) fail(`${where}: null n'est pas représentable en C++`);
	fail(`${where}: valeur invalide`);
}

// Builds a FieldView for a leaf or a comparator.
// The enum type comes from the path in enumFieldMap; ~= fills epsilon.
function toField(fieldPath: string, value: any, enumFieldMap: Record<string, string>, where: string): FieldView {
	const enumType = enumFieldMap[fieldPath];
	if (Array.isArray(value)) {
		const op = value[0];
		if (!isComparatorOp(op)) fail(`${where}: tableau inattendu`);
		const operator = OPERATOR_OF[op]!;
		if (op === "~=") {
			if (value.length !== 3) fail(`${where}: ~= attend une valeur et un seuil`);
			const threshold = value[2];
			if (typeof threshold !== "number" || !Number.isFinite(threshold) || threshold < 0) {
				fail(`${where}: le seuil de ~= doit être positif ou nul`);
			}
			const expected = cppToken(value[1], where, enumType);
			const integerNear = !expected.isEnum && Number.isInteger(value[1]) && Number.isInteger(threshold);
			return { path: fieldPath, value: expected.value, isEnum: expected.isEnum, operator, epsilon: String(threshold), integer_near: integerNear };
		}
		if (value.length !== 2) fail(`${where}: ${op} attend une valeur`);
		const token = cppToken(value[1], where, enumType);
		return { path: fieldPath, value: token.value, isEnum: token.isEnum, operator, epsilon: "", integer_near: false };
	}
	const token = cppToken(value, where, enumType);
	return { path: fieldPath, value: token.value, isEnum: token.isEnum, operator: "EQ", epsilon: "", integer_near: false };
}

// Reads $comment on the root data object.
// Splits lines, rejects control characters, and keeps a trailing backslash from joining the next line.
function commentLines(value: any, where: string): string[] {
	if (!isPlainObject(value) || !Object.hasOwn(value, "$comment")) return [];
	const comment = value["$comment"];
	if (typeof comment !== "string") fail(`${where}: "$comment" doit être une chaîne`);
	return comment.split(/\r\n|\n|\r/).map((line) => {
		for (const ch of line) {
			if (ch < " ") fail(`${where}: caractère de contrôle dans "$comment"`);
		}
		return line.endsWith("\\") ? `${line} ` : line;
	});
}

// Walks the data object of a receive or an expect_send.
// Skips a root $comment. A $comment inside a nested object is an error.
function collectFields(value: any, enumFieldMap: Record<string, string>, where: string): FieldView[] {
	if (!isPlainObject(value)) fail(`${where}: objet attendu`);
	const fields: FieldView[] = [];
	const walk = (node: Record<string, any>, prefix: string): void => {
		for (const [key, child] of Object.entries(node)) {
			if (key === "$comment") {
				if (prefix.length > 0) fail(`${where}: "$comment" n'est pas admis dans un objet imbriqué`);
				if (typeof child !== "string") fail(`${where}: "$comment" doit être une chaîne`);
				continue;
			}
			const fieldPath = prefix.length === 0 ? key : `${prefix}.${key}`;
			if (isPlainObject(child)) walk(child, fieldPath);
			else fields.push(toField(fieldPath, child, enumFieldMap, where));
		}
	};
	walk(value, "");
	return fields;
}

// Validates a step and keeps its payload as any.
// Checks the keyword and the argument count, not the shape of the data.
function asStep(value: any, where: string): Step {
	if (!Array.isArray(value) || value.length === 0) fail(`${where}: étape invalide`);
	const op = value[0];
	if (typeof op !== "string") fail(`${where}: mot-clé inconnu`);
	if (op === "reset_expect") {
		if (value.length !== 1) fail(`${where}: reset_expect n'attend aucun argument`);
		return ["reset_expect"];
	}
	if (op === "sleep_ms") {
		if (value.length !== 2 || typeof value[1] !== "number" || !Number.isFinite(value[1])) fail(`${where}: sleep_ms attend un nombre`);
		return ["sleep_ms", value[1]];
	}
	if (op === "expect_not_send") {
		if ((value.length !== 2 && value.length !== 3) || typeof value[1] !== "string") fail(`${where}: expect_not_send attend un topic`);
		if (value.length === 2) return ["expect_not_send", value[1]];
		return ["expect_not_send", value[1], value[2]];
	}
	if (op === "receive") {
		if (value.length !== 3 || typeof value[1] !== "string") fail(`${where}: receive attend un topic et des données`);
		return ["receive", value[1], value[2]];
	}
	if (op === "expect_send") {
		if (value.length !== 3 || typeof value[1] !== "string") fail(`${where}: expect_send attend un topic et une attente`);
		return ["expect_send", value[1], value[2]];
	}
	fail(`${where}: mot-clé inconnu "${op}"`);
}

// Validates one entry of tests.
// Requires name and steps, then validates each step.
function asTest(value: any, index: number): JsonTest {
	const where = `tests[${index}]`;
	if (!isPlainObject(value)) fail(`${where}: objet attendu`);
	if (typeof value.name !== "string") fail(`${where}: champ "name" manquant`);
	if (!Array.isArray(value.steps)) fail(`${where}: champ "steps" manquant`);
	const name = value.name;
	return {
		name,
		steps: value.steps.map((step: any, stepIndex: number) => asStep(step, `test "${name}", étape ${stepIndex + 1}`)),
	};
}

// Validates the document root.
// Requires name, use_virtual_time, and tests.
function asSuite(value: any): JsonTestSuite {
	if (!isPlainObject(value)) fail("la suite doit être un objet");
	if (typeof value.name !== "string") fail('champ "name" manquant');
	if (typeof value.use_virtual_time !== "boolean") fail('champ "use_virtual_time" manquant');
	if (!Array.isArray(value.tests)) fail('champ "tests" manquant');
	return {
		name: value.name,
		use_virtual_time: value.use_virtual_time,
		tests: value.tests.map((test: any, index: number) => asTest(test, index)),
	};
}

// Reads the type catalog.
// Each entry has topicList and enumFieldMap. isCan is optional and means false when absent.
function asTypeCatalog(value: any): TypeCatalog {
	if (!isPlainObject(value)) fail("types.json: objet attendu");
	const catalog: TypeCatalog = {};
	for (const [typeName, info] of Object.entries(value)) {
		if (!isPlainObject(info)) fail(`types.json: "${typeName}" doit être un objet`);
		if (!Array.isArray(info.topicList) || info.topicList.some((topic: any) => typeof topic !== "string")) {
			fail(`types.json: "${typeName}".topicList doit être une liste de chaînes`);
		}
		if (!isPlainObject(info.enumFieldMap)) fail(`types.json: "${typeName}".enumFieldMap doit être un objet`);
		let isCan = false;
		if (Object.hasOwn(info, "isCan")) {
			if (typeof info.isCan !== "boolean") fail(`types.json: "${typeName}".isCan doit être un booléen`);
			isCan = info.isCan;
		}
		const enumFieldMap: Record<string, string> = {};
		for (const [fieldPath, enumType] of Object.entries(info.enumFieldMap)) {
			if (typeof enumType !== "string") fail(`types.json: enum de "${typeName}.${fieldPath}" invalide`);
			enumFieldMap[fieldPath] = enumType;
		}
		catalog[typeName] = { topicList: info.topicList, enumFieldMap, isCan };
	}
	return catalog;
}

// Resolves a topic to its message type.
// Finds the entry whose topicList contains that topic.
function typeForTopic(catalog: TypeCatalog, topic: string, where: string): TypeInfo & { typeName: string } {
	for (const [typeName, info] of Object.entries(catalog)) {
		if (info.topicList.includes(topic)) return { typeName, ...info };
	}
	fail(`${where}: topic inconnu "${topic}"`);
}

// Formats one JSON token for the source comment.
// Keeps strings quoted and arrays in their written order.
function formatToken(value: any, where: string): string {
	if (typeof value === "string") return JSON.stringify(value);
	if (typeof value === "number") {
		if (!Number.isInteger(value)) fail(`${where}: nombre entier attendu`);
		return String(value);
	}
	if (Array.isArray(value)) return `[${value.map((item) => formatToken(item, where)).join(", ")}]`;
	fail(`${where}: valeur invalide dans le commentaire`);
}

// Restores the object as a single comment line.
// The field order is the order given, and $comment is left out.
function sourceComment(entries: Array<[string, any]>, where: string): string {
	const body = entries.map(([key, value]) => `${JSON.stringify(key)}: ${formatToken(value, where)}`).join(", ");
	const line = `{ ${body} }`;
	return line.endsWith("\\") ? `${line} ` : line;
}

// Checks a start bit.
// It must land inside a 64-bit word.
function asStart(value: any, where: string): number {
	if (typeof value !== "number" || !Number.isInteger(value) || value < 0 || value > 63) {
		fail(`${where}: "start" doit être un entier entre 0 et 63`);
	}
	return value;
}

// Checks a bit string.
// Only 0 and 1 are accepted, and the bits must fit after start.
function asBits(value: any, start: number, where: string): string {
	if (typeof value !== "string" || !/^[01]+$/.test(value)) fail(`${where}: "data" doit être une chaîne de 0 et de 1`);
	if (start + value.length > 64) fail(`${where}: les bits dépassent 64`);
	return value;
}

// Checks an id name.
// The map lookup happens afterwards.
function asIdName(value: any, where: string): string {
	if (typeof value !== "string" || value.length === 0) fail(`${where}: "id" doit être une chaîne`);
	return value;
}

// Checks a ~= threshold for an integer field.
// The threshold is a non-negative integer.
function asEpsilon(value: any, where: string): number {
	if (typeof value !== "number" || !Number.isInteger(value) || value < 0) {
		fail(`${where}: le seuil de ~= doit être un entier positif ou nul`);
	}
	return value;
}

// Reads a plain value or a comparator.
// A scalar becomes equality. ~= keeps its threshold.
function asCompared(value: any, where: string): { operator: Operator; raw: any; epsilon: number | undefined } {
	if (!Array.isArray(value)) return { operator: "EQ", raw: value, epsilon: undefined };
	const op = value[0];
	if (!isComparatorOp(op)) fail(`${where}: tableau inattendu`);
	if (op === "~=") {
		if (value.length !== 3) fail(`${where}: ~= attend une valeur et un seuil`);
		return { operator: "NEAR", raw: value[1], epsilon: asEpsilon(value[2], where) };
	}
	if (value.length !== 2) fail(`${where}: ${op} attend une valeur`);
	return { operator: OPERATOR_OF[op]!, raw: value[1], epsilon: undefined };
}

// Rejects keys other than those listed, plus $comment.
// A misplaced field is reported with the step location.
function allowKeys(value: Record<string, any>, keys: string[], where: string): void {
	for (const key of Object.keys(value)) {
		if (key === "$comment") continue;
		if (!keys.includes(key)) fail(`${where}: champ "${key}" inattendu`);
	}
}

// Reads the id map.
// Each value is a uint32 code.
function asIdMap(value: any): IdMap {
	if (!isPlainObject(value)) fail("can_ids.json: objet attendu");
	const ids: IdMap = {};
	for (const [name, code] of Object.entries(value)) {
		if (name.length === 0) fail("can_ids.json: id vide");
		if (typeof code !== "number" || !Number.isInteger(code) || code < 0 || code > 0xffff_ffff) {
			fail(`can_ids.json: code de "${name}" invalide`);
		}
		ids[name] = code;
	}
	return ids;
}

// Resolves an id through the map.
// An unknown name stops generation.
function idCodeOf(ids: IdMap, name: string, where: string): number {
	if (!Object.hasOwn(ids, name)) fail(`${where}: id inconnu "${name}"`);
	return ids[name]!;
}

// Writes a uint32 literal.
// The suffix matches the id_code field.
function u32Literal(value: number): string {
	return `${value}u`;
}

// Writes a uint64 literal.
// Hex keeps the packed bits readable.
function u64Literal(value: bigint): string {
	return `0x${value.toString(16).toUpperCase()}ULL`;
}

// Writes an integer threshold with the field's suffix.
function epsilonLiteral(value: number, suffix: "u" | "ULL"): string {
	return `${value}${suffix}`;
}

// Places a bit string with the left bit as the high bit.
// The result is then shifted by start.
function packMsb(bits: string, start: number): bigint {
	let value = 0n;
	for (const ch of bits) value = (value << 1n) | (ch === "1" ? 1n : 0n);
	return value << BigInt(start);
}

// Places a bit string with the first character at start.
// The following characters occupy the higher bits.
function packLsb(bits: string, start: number): bigint {
	let value = 0n;
	for (let i = 0; i < bits.length; i++) {
		if (bits[i] === "1") value |= 1n << BigInt(start + i);
	}
	return value;
}

// Chooses the bit order from the process arguments.
// MSB is the default. --lsb selects the other order.
function packFromArgs(args: string[]): Pack {
	if (args.length === 0) return packMsb;
	if (args.length === 1 && args[0] === "--lsb") return packLsb;
	fail(`argument inconnu "${args.join(" ")}"`);
}

// Reads id, start, and data for a receive.
// Operators are rejected: a receive assigns exact values.
function receivePayload(value: any, ids: IdMap, pack: Pack, where: string): { id: string; code: number; data: bigint; source: string; comments: string[] } {
	if (!isPlainObject(value)) fail(`${where}: objet attendu`);
	allowKeys(value, ["id", "start", "data"], where);
	if (!Object.hasOwn(value, "id") || !Object.hasOwn(value, "start") || !Object.hasOwn(value, "data")) {
		fail(`${where}: "id", "start" et "data" sont requis`);
	}
	const id = asIdName(value.id, where);
	const start = asStart(value.start, where);
	const bits = asBits(value.data, start, where);
	return {
		id,
		code: idCodeOf(ids, id, where),
		data: pack(bits, start),
		source: sourceComment(
			[
				["id", value.id],
				["start", value.start],
				["data", value.data],
			],
			where,
		),
		comments: commentLines(value, where),
	};
}

// Reads an expect_send object.
// id and data may be comparators; start only places the bits.
function expectPayload(
	value: any,
	ids: IdMap,
	pack: Pack,
	where: string,
): { id: string; code: number; fields: FieldView[]; source: string; comments: string[] } {
	if (!isPlainObject(value)) fail(`${where}: objet attendu`);
	allowKeys(value, ["id", "start", "data"], where);
	if (!Object.hasOwn(value, "id") || !Object.hasOwn(value, "start") || !Object.hasOwn(value, "data")) {
		fail(`${where}: "id", "start" et "data" sont requis`);
	}
	const start = asStart(value.start, where);
	const idCmp = asCompared(value.id, `${where}: "id"`);
	const id = asIdName(idCmp.raw, `${where}: "id"`);
	const code = idCodeOf(ids, id, where);
	const dataCmp = asCompared(value.data, `${where}: "data"`);
	const bits = asBits(dataCmp.raw, start, `${where}: "data"`);
	const fields: FieldView[] = [
		{
			path: "id_code",
			value: u32Literal(code),
			isEnum: false,
			operator: idCmp.operator,
			epsilon: idCmp.epsilon === undefined ? "" : epsilonLiteral(idCmp.epsilon, "u"),
			integer_near: idCmp.epsilon !== undefined,
		},
		{
			path: "data",
			value: u64Literal(pack(bits, start)),
			isEnum: false,
			operator: dataCmp.operator,
			epsilon: dataCmp.epsilon === undefined ? "" : epsilonLiteral(dataCmp.epsilon, "ULL"),
			integer_near: dataCmp.epsilon !== undefined,
		},
	];
	return {
		id,
		code,
		fields,
		source: sourceComment(
			[
				["id", value.id],
				["start", value.start],
				["data", value.data],
			],
			where,
		),
		comments: commentLines(value, where),
	};
}

// Reads an expect_not_send object.
// Only the id is used to build the key.
function absentPayload(value: any, ids: IdMap, where: string): { id: string; code: number; source: string; comments: string[] } {
	if (!isPlainObject(value)) fail(`${where}: objet attendu`);
	allowKeys(value, ["id"], where);
	if (!Object.hasOwn(value, "id")) fail(`${where}: "id" est requis`);
	const id = asIdName(value.id, where);
	return {
		id,
		code: idCodeOf(ids, id, where),
		source: sourceComment([["id", value.id]], where),
		comments: commentLines(value, where),
	};
}

// Builds an empty CAN view.
// Every string is blank so unused Mustache sections stay hidden.
function emptyCan(): CanView {
	return {
		topic_with_slash: "",
		id_code_number: "",
		id_code: "",
		data: "",
		id_text: "",
		source_comment: "",
	};
}

// Builds the CAN view for a receive.
// id_code and data are exact. The lookup key stays empty.
function canReceive(value: any, ids: IdMap, pack: Pack, where: string): { can: CanView; comments: string[] } {
	const payload = receivePayload(value, ids, pack, where);
	return {
		can: {
			...emptyCan(),
			id_code: u32Literal(payload.code),
			data: u64Literal(payload.data),
			source_comment: payload.source,
		},
		comments: payload.comments,
	};
}

// Builds the CAN view for an expect_send.
// The topic always gains a trailing slash for the VariantMap key.
function canExpect(topic: string, value: any, ids: IdMap, pack: Pack, where: string): { can: CanView; comments: string[]; fields: FieldView[] } {
	const payload = expectPayload(value, ids, pack, where);
	return {
		can: {
			...emptyCan(),
			topic_with_slash: cppString(`${topic}/`, where),
			id_code_number: String(payload.code),
			id_text: cppStringContents(payload.id, where),
			source_comment: payload.source,
		},
		comments: payload.comments,
		fields: payload.fields,
	};
}

// Builds the CAN view for an expect_not_send.
// Only the id selects the key. No field comparisons are emitted.
function canAbsent(topic: string, value: any, ids: IdMap, where: string): { can: CanView; comments: string[] } {
	const payload = absentPayload(value, ids, where);
	return {
		can: {
			...emptyCan(),
			topic_with_slash: cppString(`${topic}/`, where),
			id_code_number: String(payload.code),
			id_text: cppStringContents(payload.id, where),
			source_comment: payload.source,
		},
		comments: payload.comments,
	};
}

// Prepares a step view whose Mustache sections are false.
// blank_before is the only flag already decided.
function emptyStep(blankBeforeStep: boolean): StepView {
	return {
		blank_before: blankBeforeStep,
		is_reset: false,
		is_sleep: false,
		is_receive: false,
		is_expect_send: false,
		is_expect_not_send: false,
		is_can: false,
		ms: "",
		type_name: "",
		topic_pascal: "",
		var_name: "",
		topic_key: "",
		comment_lines: [],
		fields: [],
		can: emptyCan(),
	};
}

// Decides whether a blank line precedes the step.
// It is omitted for the first step, and when expect_not_send immediately follows reset_expect.
function blankBefore(steps: Step[], index: number): boolean {
	if (index === 0) return false;
	const prev = steps[index - 1];
	const cur = steps[index];
	if (prev?.[0] === "reset_expect" && cur?.[0] === "expect_not_send") return false;
	return true;
}

// Turns a step into its Mustache view.
// Numbers data_in_N and data_out_N. A CAN topic packs bits; any other topic walks fields.
function toStepView(
	step: Step,
	index: number,
	steps: Step[],
	counters: { dataIn: number; dataOut: number },
	catalog: TypeCatalog,
	ids: IdMap,
	pack: Pack,
	testName: string,
): StepView {
	const where = `test "${testName}", étape ${index + 1}`;
	const view = emptyStep(blankBefore(steps, index));
	switch (step[0]) {
		case "reset_expect":
			view.is_reset = true;
			return view;
		case "sleep_ms":
			view.is_sleep = true;
			view.ms = String(step[1]);
			return view;
		case "receive": {
			const info = typeForTopic(catalog, step[1], where);
			view.is_receive = true;
			view.type_name = info.typeName;
			view.topic_pascal = toCppName(step[1], where);
			view.var_name = `data_in_${++counters.dataIn}`;
			if (info.isCan) {
				const built = canReceive(step[2], ids, pack, where);
				view.is_can = true;
				view.can = built.can;
				view.comment_lines = built.comments;
				return view;
			}
			view.comment_lines = commentLines(step[2], where);
			view.fields = collectFields(step[2], info.enumFieldMap, where);
			return view;
		}
		case "expect_send": {
			const info = typeForTopic(catalog, step[1], where);
			view.is_expect_send = true;
			view.type_name = info.typeName;
			view.var_name = `data_out_${++counters.dataOut}`;
			if (info.isCan) {
				const built = canExpect(step[1], step[2], ids, pack, where);
				view.is_can = true;
				view.can = built.can;
				view.comment_lines = built.comments;
				view.fields = built.fields;
				return view;
			}
			view.topic_key = step[1];
			view.comment_lines = commentLines(step[2], where);
			view.fields = collectFields(step[2], info.enumFieldMap, where);
			return view;
		}
		case "expect_not_send": {
			const info = typeForTopic(catalog, step[1], where);
			view.is_expect_not_send = true;
			view.type_name = info.typeName;
			view.var_name = `data_out_${++counters.dataOut}`;
			if (info.isCan) {
				if (step.length !== 3) fail(`${where}: expect_not_send attend un topic et un id`);
				const built = canAbsent(step[1], step[2], ids, where);
				view.is_can = true;
				view.can = built.can;
				view.comment_lines = built.comments;
				return view;
			}
			if (step.length !== 2) fail(`${where}: expect_not_send attend un topic`);
			view.topic_key = step[1];
			return view;
		}
	}
}

// Assembles the view for the C++ file.
// Resets the variable counters at the start of each test.
function toView(suite: JsonTestSuite, catalog: TypeCatalog, ids: IdMap, pack: Pack): View {
	return {
		suite_class: toCppName(suite.name, "nom de suite"),
		use_virtual_time: suite.use_virtual_time,
		tests: suite.tests.map((test) => {
			const counters = { dataIn: 0, dataOut: 0 };
			return {
				method: toCppName(test.name, `test "${test.name}"`),
				steps: test.steps.map((step, index) => toStepView(step, index, test.steps, counters, catalog, ids, pack, test.name)),
			};
		}),
	};
}

// Loads a JSON file.
// Strips a leading BOM, then calls JSON.parse, which does not keep number spelling.
function readJson(filePath: string): any {
	let text = fs.readFileSync(filePath, "utf-8");
	if (text.charCodeAt(0) === 0xfeff) text = text.slice(1);
	return JSON.parse(text);
}

if (import.meta.main) {
	const pack = packFromArgs(process.argv.slice(2));
	const root = path.join(import.meta.dir, "..");
	const suite = asSuite(readJson(path.join(root, "example1.json")));
	const catalog = asTypeCatalog(readJson(path.join(root, "types.json")));
	const ids = asIdMap(readJson(path.join(root, "can_ids.json")));
	const view: View = toView(suite, catalog, ids, pack);
	const google_tests_content = mustache
		.render(tpl, view, undefined, { escape: (v) => v })
		.replace(/\\\r?\n/g, "")
		.replace(/\r?\n/g, "\r\n");
	fs.writeFileSync(path.join(root, "src", "gen", "example1_test.cpp"), google_tests_content);
}
