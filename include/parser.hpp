#ifndef HADRON_PARSER_H
#define HADRON_PARSER_H 1

#include "lexer.hpp"
#include "symbol.hpp"
#include "types.hpp"
#include "vm.hpp"


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
  bool        match(Type type);

  void parse_fxn(const Token &);
  void parse_lit(const Token &);
  void parse_err(const Token &);
  void parse_unr(const Token &);
  void parse_bin(const Token &);
  void parse_grp(const Token &);
  void parse_rng(const Token &);
  void parse_dcl(const Token &);

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
