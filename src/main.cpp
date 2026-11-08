#include "cpp-terminal/exception.hpp"
#include "cpp-terminal/input.hpp"
#include "cpp-terminal/iostream.hpp"
#include "cpp-terminal/key.hpp"
#include "cpp-terminal/options.hpp"
#include "cpp-terminal/terminal.hpp"
#include "cpp-terminal/tty.hpp"

int main() {
  try {
    Term::terminal.setOptions(Term::Option::NoClearScreen, Term::Option::NoSignalKeys,
                              Term::Option::Cursor, Term::Option::Raw);
    if (!Term::is_stdin_a_tty()) {
      Term::cerr << "Le terminal n'est pas un TTY." << std::endl;
      return 2;
    }

    Term::cout << "Appuyez sur une touche (q pour quitter) :" << std::endl;
    while (true) {
      const Term::Event event = Term::read_event();
      if (event.type() != Term::Event::Type::Key) {
        continue;
      }

      const Term::Key key(event);
      Term::cout << key.name() << std::endl;
      if (key == Term::Key::q) {
        break;
      }
    }
  } catch (const Term::Exception& error) {
    Term::cerr << error.what() << std::endl;
    return 2;
  }

  return 0;
}
