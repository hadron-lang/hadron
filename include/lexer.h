#ifndef HADRON_LEXER_H
#define HADRON_LEXER_H 1

#include "input.h"
#include "types.h"

class Lexer {
  bool   end{false};
  bool   had_float{false}; // State-tracking for range operator parsing
  char   current_char{'\0'};
  char   next_char{'\0'};
  int    iterator{-1};
  int    character{1};
  int    line{1};
  int    start{0};
  int    absStart{0};

  char               next();
  [[nodiscard]] char current() const;
  [[nodiscard]] char peek() const;
  [[nodiscard]] char peek2() const;
  [[nodiscard]] bool match(char c);

  [[nodiscard]] Token emit(Type type);
  [[nodiscard]] Token emit(Type type, double value) const;
  [[nodiscard]] Token number(char first_char);

  public:
  Input &input;
  explicit Lexer(Input &input) : input(input) {
    next_char = input.next(); // manually load the first char
  }
  void  reset(const Input &input);
  Token advance();
};

#endif // HADRON_LEXER_H
