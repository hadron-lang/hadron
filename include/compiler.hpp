#ifndef HADRON_COMPILER_H
#define HADRON_COMPILER_H 1

#include "lexer.hpp"

typedef struct Compiler {
  Lexer *lexer;

  explicit Compiler(Lexer *lexer) : lexer(lexer) {}
} Compiler;

#endif // HADRON_COMPILER_H
