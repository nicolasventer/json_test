#pragma once

#include "IModule.hpp"
#include "jwrap/jwrap.hpp"

namespace jw {

template <>
struct Converter<EStateStatus> {
	static Value encode(EStateStatus value) {
		const char* name = nullptr;
		switch (value) {
		case INIT:
			name = "INIT";
			break;
		case RUNNING:
			name = "RUNNING";
			break;
		case CLOSED:
			name = "CLOSED";
			break;
		}
		if (name == nullptr) {
			throw type_error("Valeur d'enum inconnue");
		}
		return Converter<std::string>::encode(name);
	}

	static EStateStatus decode(const Value& value) {
		if (!value.is_string()) {
			throw type_error("EStateStatus attend une chaîne");
		}
		const std::string text = Converter<std::string>::decode(value);
		if (text == "INIT") return INIT;
		if (text == "RUNNING") return RUNNING;
		if (text == "CLOSED") return CLOSED;
		throw type_error("Nom d'enum inconnu : " + text);
	}
};

template <>
struct Converter<EAlarmLevel> {
	static Value encode(EAlarmLevel value) {
		const char* name = nullptr;
		switch (value) {
		case INFO:
			name = "INFO";
			break;
		case WARN:
			name = "WARN";
			break;
		case FAULT:
			name = "FAULT";
			break;
		}
		if (name == nullptr) {
			throw type_error("Valeur d'enum inconnue");
		}
		return Converter<std::string>::encode(name);
	}

	static EAlarmLevel decode(const Value& value) {
		if (!value.is_string()) {
			throw type_error("EAlarmLevel attend une chaîne");
		}
		const std::string text = Converter<std::string>::decode(value);
		if (text == "INFO") return INFO;
		if (text == "WARN") return WARN;
		if (text == "FAULT") return FAULT;
		throw type_error("Nom d'enum inconnu : " + text);
	}
};

template <>
struct Converter<cmd> {
	static Value encode(const cmd& value) {
		Value result = Value::object();
		result.set("value", value.value);
		return result;
	}

	static cmd decode(const Value& value) {
		if (!value.is_object()) {
			throw type_error("cmd attend un objet");
		}
		cmd result;
		result.value = value.get<std::string>("value");
		return result;
	}
};

template <>
struct Converter<config> {
	static Value encode(const config& value) {
		Value result = Value::object();
		result.set("interval_ms", value.interval_ms);
		result.set("threshold", value.threshold);
		return result;
	}

	static config decode(const Value& value) {
		if (!value.is_object()) {
			throw type_error("config attend un objet");
		}
		config result{};
		result.interval_ms = value.get<int>("interval_ms");
		result.threshold = value.get<double>("threshold");
		return result;
	}
};

template <>
struct Converter<state> {
	static Value encode(const state& value) {
		Value result = Value::object();
		result.set("status", value.status);
		result.set("uptime_ms", value.uptime_ms);
		return result;
	}

	static state decode(const Value& value) {
		if (!value.is_object()) {
			throw type_error("state attend un objet");
		}
		state result{};
		result.status = value.get<EStateStatus>("status");
		result.uptime_ms = value.get<int>("uptime_ms");
		return result;
	}
};

template <>
struct Converter<measure> {
	static Value encode(const measure& value) {
		Value result = Value::object();
		result.set("temperature", value.temperature);
		result.set("humidity", value.humidity);
		return result;
	}

	static measure decode(const Value& value) {
		if (!value.is_object()) {
			throw type_error("measure attend un objet");
		}
		measure result{};
		result.temperature = value.get<double>("temperature");
		result.humidity = value.get<int>("humidity");
		return result;
	}
};

template <>
struct Converter<alarm> {
	static Value encode(const alarm& value) {
		Value result = Value::object();
		result.set("level", value.level);
		result.set("code", value.code);
		return result;
	}

	static alarm decode(const Value& value) {
		if (!value.is_object()) {
			throw type_error("alarm attend un objet");
		}
		alarm result{};
		result.level = value.get<EAlarmLevel>("level");
		result.code = value.get<int>("code");
		return result;
	}
};

}  // namespace jw
