#include "async_call.hpp"
#include "console_module.hpp"
#include "device_logic.hpp"

#include "cpp-terminal/event.hpp"
#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/input.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/key.hpp"

#include <chrono>
#include <iostream>

int main() {
  try {
    DeviceLogic logic;
    ConsoleModule module{logic};
    module.Start();

    auto showKeys = [] {
      std::cout << R"(a: cmd { "value": "START" })" << std::endl;
      std::cout << R"(b: config { "interval_ms": 100, "threshold": 30.5 })" << std::endl;
      std::cout << "échap: quitter" << std::endl;
      std::cout << "tab: réafficher les touches" << std::endl;
    };
    showKeys();
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
        if (*key == Term::Key::Tab) {
          showKeys();
        } else if (*key == Term::Key::a) {
          std::cout << R"(receive cmd { "value": "START" })" << std::endl;
          logic.onReceive_cmd(cmd{"START"});
        } else if (*key == Term::Key::b) {
          std::cout << R"(receive config { "interval_ms": 100, "threshold": 30.5 })" << std::endl;
          logic.onReceive_config(config{100, 30.5});
        } else {
          std::cout << "receive unknown key '" << key->name() << "'" << std::endl;
        }
      } else {
        module.getTimer().sleep(std::chrono::milliseconds{20});
      }
    }
  } catch (const Term::Exception& error) {
    Term::cerr << error.what() << std::endl;
    return 2;
  }

  return 0;
}
