import fs from "fs";
import path from "path";
import mustache from "mustache";
// @ts-expect-error cannot handle raw
import tpl from "./google_tests.cpp.mustache?raw";

type Operator = "EQ" | "NE" | "GT" | "GE" | "LT" | "LE" | "NEAR";

type Step =
	| ["reset_expect"]
	| ["sleep_ms", number]
	| ["receive", string, any]
	| ["expect_send", string, any]
	| ["expect_not_send", string];

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
};

type TypeCatalog = Record<string, TypeInfo>;

type FieldView = {
	path: string;
	value: string;
	isEnum: boolean;
	operator: Operator;
	epsilon: string;
};

type StepView = {
	blank_before: boolean;
	is_reset: boolean;
	is_sleep: boolean;
	is_receive: boolean;
	is_expect_send: boolean;
	is_expect_not_send: boolean;
	ms: string;
	type_name: string;
	var_name: string;
	topic_key: string;
	fields: FieldView[];
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
			return { path: fieldPath, value: expected.value, isEnum: expected.isEnum, operator, epsilon: String(threshold) };
		}
		if (value.length !== 2) fail(`${where}: ${op} attend une valeur`);
		const token = cppToken(value[1], where, enumType);
		return { path: fieldPath, value: token.value, isEnum: token.isEnum, operator, epsilon: "" };
	}
	const token = cppToken(value, where, enumType);
	return { path: fieldPath, value: token.value, isEnum: token.isEnum, operator: "EQ", epsilon: "" };
}

// Walks the data object of a receive or an expect_send.
// Nested objects extend the path with a dot.
function collectFields(value: any, enumFieldMap: Record<string, string>, where: string): FieldView[] {
	if (!isPlainObject(value)) fail(`${where}: objet attendu`);
	const fields: FieldView[] = [];
	const walk = (node: Record<string, any>, prefix: string): void => {
		for (const [key, child] of Object.entries(node)) {
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
		if (value.length !== 2 || typeof value[1] !== "string") fail(`${where}: expect_not_send attend un topic`);
		return ["expect_not_send", value[1]];
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
// Each entry has topicList and enumFieldMap, with no check on names.
function asTypeCatalog(value: any): TypeCatalog {
	if (!isPlainObject(value)) fail("types.json: objet attendu");
	const catalog: TypeCatalog = {};
	for (const [typeName, info] of Object.entries(value)) {
		if (!isPlainObject(info)) fail(`types.json: "${typeName}" doit être un objet`);
		if (!Array.isArray(info.topicList) || info.topicList.some((topic: any) => typeof topic !== "string")) {
			fail(`types.json: "${typeName}".topicList doit être une liste de chaînes`);
		}
		if (!isPlainObject(info.enumFieldMap)) fail(`types.json: "${typeName}".enumFieldMap doit être un objet`);
		const enumFieldMap: Record<string, string> = {};
		for (const [fieldPath, enumType] of Object.entries(info.enumFieldMap)) {
			if (typeof enumType !== "string") fail(`types.json: enum de "${typeName}.${fieldPath}" invalide`);
			enumFieldMap[fieldPath] = enumType;
		}
		catalog[typeName] = { topicList: info.topicList, enumFieldMap };
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
		ms: "",
		type_name: "",
		var_name: "",
		topic_key: "",
		fields: [],
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
// Numbers data_in_N and data_out_N, and qualifies enums from enumFieldMap.
function toStepView(step: Step, index: number, steps: Step[], counters: { dataIn: number; dataOut: number }, catalog: TypeCatalog, testName: string): StepView {
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
			view.var_name = `data_in_${++counters.dataIn}`;
			view.fields = collectFields(step[2], info.enumFieldMap, where);
			return view;
		}
		case "expect_send": {
			const info = typeForTopic(catalog, step[1], where);
			view.is_expect_send = true;
			view.type_name = info.typeName;
			view.topic_key = info.typeName;
			view.var_name = `data_out_${++counters.dataOut}`;
			view.fields = collectFields(step[2], info.enumFieldMap, where);
			return view;
		}
		case "expect_not_send": {
			const info = typeForTopic(catalog, step[1], where);
			view.is_expect_not_send = true;
			view.type_name = info.typeName;
			view.topic_key = info.typeName;
			view.var_name = `data_out_${++counters.dataOut}`;
			return view;
		}
	}
}

// Assembles the view for the C++ file.
// Resets the variable counters at the start of each test.
function toView(suite: JsonTestSuite, catalog: TypeCatalog): View {
	return {
		suite_class: toCppName(suite.name, "nom de suite"),
		use_virtual_time: suite.use_virtual_time,
		tests: suite.tests.map((test) => {
			const counters = { dataIn: 0, dataOut: 0 };
			return {
				method: toCppName(test.name, `test "${test.name}"`),
				steps: test.steps.map((step, index) => toStepView(step, index, test.steps, counters, catalog, test.name)),
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
	const root = path.join(import.meta.dir, "..");
	const suite = asSuite(readJson(path.join(root, "example1.json")));
	const catalog = asTypeCatalog(readJson(path.join(root, "types.json")));
	const view: View = toView(suite, catalog);
	const google_tests_content = mustache
		.render(tpl, view, undefined, { escape: (v) => v })
		.replace(/\\\r?\n/g, "")
		.replace(/\r?\n/g, "\r\n");
	fs.writeFileSync(path.join(root, "src", "gen", "example1_test.cpp"), google_tests_content);
}
