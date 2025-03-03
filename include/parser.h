#ifndef HADRON_PARSER_H
#define HADRON_PARSER_H 1

#include "lexer.h"
#include "symbol.h"
#include "types.h"
#include "vm.h"

enum class Precedence : int8_t {
  NUL = -1,
  LIT,
  ASG = 1,
  LOR,
  LND,
  BOR,
  XOR,
  BND,
  EQT,
  CMP,
  BSH,
  TRM,
  FCT,
  EXP,
  RNG,
  UNR,
  GRP,
  MAX,
};

typedef class Parser {
  public:
  Lexer      &lexer;
  Chunk      &chunk;
  Token       current_token{};
  Token       prev_token{};
  SymbolTable symbols;

  explicit Parser(Lexer &lexer, Chunk &chunk);
  void   advance();
  Token &consume(Type type, const char *error);
  bool   match(Type type);

  void parse();
  void parse_expression(Precedence precedence);
} Parser;

typedef void (*NudFn)(Parser &, const Token &);
typedef void (*LedFn)(Parser &, const Token &);

typedef struct ParseRule {
  Precedence precedence;
  NudFn      nud;
  LedFn      led;
} ParseRule;

#endif // HADRON_PARSER_H
