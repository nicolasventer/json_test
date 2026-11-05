#include "suite_runner.hpp"

#include "device_logic.hpp"
#include "module_expect.hpp"
#include "module_json.hpp"
#include "test_module.hpp"

#include <chrono>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace json_test {

enum class StepKind {
  ResetExpect,
  Receive,
  ElapseMs,
  ExpectSend,
  ExpectNotSend,
};

struct Step {
  StepKind kind = StepKind::ResetExpect;
  std::string topic;
  jw::Value payload;
  int milliseconds = 0;
};

struct SuiteTest {
  std::string name;
  int expect_timeout_ms = 500;
  bool skipped = false;
  std::string skip_reason;
  std::vector<Step> steps;
};

struct TestSuite {
  std::string name;
  bool b_random = false;
  std::vector<SuiteTest> tests;
};

namespace {

const char* keyword_of(StepKind kind) {
  switch (kind) {
    case StepKind::ResetExpect:
      return "reset_expect";
    case StepKind::Receive:
      return "receive";
    case StepKind::ElapseMs:
      return "elapse_ms";
    case StepKind::ExpectSend:
      return "expect_send";
    case StepKind::ExpectNotSend:
      return "expect_not_send";
  }
  return "etape";
}

std::string read_file(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    throw std::runtime_error("Impossible de lire le fichier : " + path);
  }

  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

bool is_operator_name(const std::string& name) {
  return name == "=" || name == "!=" || name == ">" || name == ">=" || name == "<" || name == "<=";
}

bool read_operator(const jw::Value& value, std::string& name, jw::Value& rhs) {
  if (!value.is_array() || value.size() != 2 || !value[0].is_string()) {
    return false;
  }

  name = value[0].get<std::string>();
  if (!is_operator_name(name)) {
    return false;
  }

  rhs = value[1];
  return true;
}

double as_number(const jw::Value& value) { return yyjson_mut_get_num(value.mut_val()); }

bool apply_operator(const jw::Value& actual, const std::string& op, const jw::Value& rhs) {
  if (actual.is_number() && rhs.is_number()) {
    const double left = as_number(actual);
    const double right = as_number(rhs);
    if (op == "=") return left == right;
    if (op == "!=") return left != right;
    if (op == ">") return left > right;
    if (op == ">=") return left >= right;
    if (op == "<") return left < right;
    if (op == "<=") return left <= right;
  }

  if (actual.is_string() && rhs.is_string() && (op == "=" || op == "!=")) {
    const bool equal = actual.get<std::string>() == rhs.get<std::string>();
    return op == "=" ? equal : !equal;
  }

  return false;
}

bool matches(const jw::Value& actual, const jw::Value& expected);

bool matches_array(const jw::Value& actual, const jw::Value& expected) {
  std::string op;
  jw::Value rhs = jw::Value::null();
  if (read_operator(expected, op, rhs)) {
    return apply_operator(actual, op, rhs);
  }

  if (!actual.is_array() || actual.size() != expected.size()) {
    return false;
  }

  for (std::size_t i = 0; i < expected.size(); ++i) {
    if (!matches(actual[i], expected[i])) {
      return false;
    }
  }
  return true;
}

bool matches(const jw::Value& actual, const jw::Value& expected) {
  if (expected.is_object()) {
    if (!actual.is_object()) {
      return false;
    }

    const auto fields = expected.get<std::map<std::string, jw::Value>>();
    for (const auto& field : fields) {
      if (!actual.has_key(field.first) || !matches(actual[field.first], field.second)) {
        return false;
      }
    }
    return true;
  }

  if (expected.is_array()) {
    return matches_array(actual, expected);
  }

  if (expected.is_null() || actual.is_null()) {
    return expected.is_null() && actual.is_null();
  }

  if (expected.is_bool() || actual.is_bool()) {
    return expected.is_bool() && actual.is_bool() && expected.get<bool>() == actual.get<bool>();
  }

  if (expected.is_number() || actual.is_number()) {
    return expected.is_number() && actual.is_number() && as_number(expected) == as_number(actual);
  }

  if (expected.is_string() || actual.is_string()) {
    return expected.is_string() && actual.is_string() &&
           expected.get<std::string>() == actual.get<std::string>();
  }

  return false;
}

std::string step_failure(const SuiteTest& test, std::size_t index, const Step& step, const std::string& detail) {
  std::ostringstream out;
  out << "Test \"" << test.name << "\", étape " << (index + 1) << " (" << keyword_of(step.kind);
  if (!step.topic.empty()) {
    out << " " << step.topic;
  }
  out << ", timeout " << test.expect_timeout_ms << " ms) : " << detail;
  return out.str();
}

std::string expect_send_failure(const SuiteTest& test, std::size_t index, const Step& step,
                                const std::vector<IModuleExpect::Sent>& sent) {
  std::ostringstream detail;
  detail << "aucun message correspondant.\nAttendu : " << jw::stringify(step.payload) << "\nMessages :";

  bool any = false;
  for (const auto& message : sent) {
    if (message.topic != step.topic) {
      continue;
    }
    any = true;
    detail << "\n" << message.topic << " " << jw::stringify(message.data);
  }

  if (!any) {
    detail << "\n(aucun " << step.topic << ")";
  }

  return step_failure(test, index, step, detail.str());
}

}  // namespace
}  // namespace json_test

namespace jw {

template <>
struct Converter<json_test::Step> {
  static json_test::Step decode(const Value& value) {
    if (!value.is_array() || value.size() < 1 || !value[0].is_string()) {
      throw type_error("Une étape doit être un tableau commençant par un mot-clé");
    }

    const std::string keyword = value.at<std::string>(0);
    json_test::Step step;

    if (keyword == "reset_expect") {
      if (value.size() != 1) {
        throw type_error("Nombre d'arguments invalide pour reset_expect");
      }
      step.kind = json_test::StepKind::ResetExpect;
      return step;
    }

    if (keyword == "elapse_ms") {
      if (value.size() != 2) {
        throw type_error("Nombre d'arguments invalide pour elapse_ms");
      }
      step.kind = json_test::StepKind::ElapseMs;
      step.milliseconds = value.at<int>(1);
      return step;
    }

    if (keyword == "expect_not_send") {
      if (value.size() != 2 || !value[1].is_string()) {
        throw type_error("Nombre d'arguments invalide pour expect_not_send");
      }
      step.kind = json_test::StepKind::ExpectNotSend;
      step.topic = value.at<std::string>(1);
      return step;
    }

    if (keyword == "receive" || keyword == "expect_send") {
      if (value.size() != 3 || !value[1].is_string()) {
        throw type_error("Nombre d'arguments invalide pour " + keyword);
      }
      step.kind = keyword == "receive" ? json_test::StepKind::Receive : json_test::StepKind::ExpectSend;
      step.topic = value.at<std::string>(1);
      step.payload = value[2];
      return step;
    }

    throw type_error("Mot-clé d'étape inconnu : " + keyword);
  }

  static Value encode(const json_test::Step&) { throw error("L'encodage d'une étape n'est pas pris en charge"); }
};

template <>
struct Converter<json_test::SuiteTest> {
  static json_test::SuiteTest decode(const Value& value) {
    if (!value.is_object()) {
      throw type_error("Un test doit être un objet");
    }

    json_test::SuiteTest test;
    test.name = value.get<std::string>("name");
    test.steps = value.get<std::vector<json_test::Step>>("steps");

    if (value.has_key("expect_timeout_ms")) {
      test.expect_timeout_ms = value.get<int>("expect_timeout_ms");
      if (test.expect_timeout_ms <= 0) {
        throw type_error("expect_timeout_ms doit être un entier positif");
      }
    }

    if (value.has_key("skip")) {
      const Value skip = value["skip"];
      if (skip.is_bool()) {
        test.skipped = skip.get<bool>();
      } else if (skip.is_string()) {
        test.skipped = true;
        test.skip_reason = skip.get<std::string>();
      } else {
        throw type_error("skip doit être un booléen ou une chaîne");
      }
    }

    return test;
  }

  static Value encode(const json_test::SuiteTest&) { throw error("L'encodage d'un test n'est pas pris en charge"); }
};

template <>
struct Converter<json_test::TestSuite> {
  static json_test::TestSuite decode(const Value& value) {
    if (!value.is_object()) {
      throw type_error("La suite doit être un objet");
    }

    json_test::TestSuite suite;
    suite.name = value.get<std::string>("name");
    suite.b_random = value.get<bool>("b_random");
    suite.tests = value.get<std::vector<json_test::SuiteTest>>("tests");
    return suite;
  }

  static Value encode(const json_test::TestSuite&) { throw error("L'encodage d'une suite n'est pas pris en charge"); }
};

}  // namespace jw

namespace json_test {

namespace {

TestSuite load_suite(const std::string& suite_path) {
  const jw::Value root = jw::Value::parse(read_file(suite_path));
  return jw::from_json<TestSuite>(root);
}

}  // namespace

std::vector<std::string> list_test_names(const std::string& suite_path) {
  const TestSuite suite = load_suite(suite_path);
  std::vector<std::string> names;
  names.reserve(suite.tests.size());
  for (const SuiteTest& test : suite.tests) {
    names.push_back(test.name);
  }
  return names;
}

RunResult run_loaded(const TestSuite& suite, const std::string& test_name, IModuleLogic& logic, IModule& module,
                     IModuleExpect& expect, Timer& timer) {
  (void)suite.b_random;

  const SuiteTest* selected = nullptr;
  for (const auto& test : suite.tests) {
    if (test.name == test_name) {
      selected = &test;
      break;
    }
  }

  if (selected == nullptr) {
    throw std::runtime_error("Test introuvable : " + test_name);
  }

  RunResult result;
  if (selected->skipped) {
    result.skipped = true;
    result.skip_reason = selected->skip_reason.empty() ? "test ignoré" : selected->skip_reason;
    return result;
  }

  module.setLogic(&logic);
  module.Start();

  for (std::size_t index = 0; index < selected->steps.size(); ++index) {
    const Step& step = selected->steps[index];
    try {
      switch (step.kind) {
        case StepKind::ResetExpect:
          expect.resetExpect();
          break;
        case StepKind::ElapseMs:
          timer.sleep(std::chrono::milliseconds{step.milliseconds});
          break;
        case StepKind::Receive:
          if (step.topic == "cmd") {
            logic.onReceive(step.payload.get<cmd>());
          } else if (step.topic == "config") {
            logic.onReceive(step.payload.get<config>());
          } else {
            result.failure = step_failure(*selected, index, step, "topic reçu inconnu");
            return result;
          }
          break;
        case StepKind::ExpectSend: {
          bool found = false;
          for (const auto& message : expect.sent()) {
            if (message.topic == step.topic && matches(message.data, step.payload)) {
              found = true;
              break;
            }
          }
          if (!found) {
            result.failure = expect_send_failure(*selected, index, step, expect.sent());
            return result;
          }
          break;
        }
        case StepKind::ExpectNotSend:
          for (std::size_t sent_index = expect.resetIndex(); sent_index < expect.sent().size(); ++sent_index) {
            if (expect.sent()[sent_index].topic == step.topic) {
              result.failure = step_failure(
                  *selected, index, step,
                  "message envoyé : " + jw::stringify(expect.sent()[sent_index].data));
              return result;
            }
          }
          break;
      }
    } catch (const std::exception& ex) {
      result.failure = step_failure(*selected, index, step, ex.what());
      return result;
    }
  }

  return result;
}

RunResult run_named_test(const std::string& suite_path, const std::string& test_name) {
  try {
    const TestSuite suite = load_suite(suite_path);
    Timer timer;
    TestModule module(timer);
    DeviceLogic logic(module, timer);
    return run_loaded(suite, test_name, logic, module, module, timer);
  } catch (const std::exception& ex) {
    RunResult result;
    result.failure = ex.what();
    return result;
  }
}

}  // namespace json_test
