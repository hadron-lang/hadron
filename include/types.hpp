#ifndef HADRON_TYPES_H
#define HADRON_TYPES_H 1

#include <cstdint>

typedef enum class Types : uint8_t {
  ERROR,

  // COMPARE
  CMP_EQ,
  CMP_NEQ,
  CMP_GT,
  CMP_GEQ,
  CMP_LT,
  CMP_LEQ,

  // ASSIGN
  CST_EQ,
  SET_EQ,
  EQ,
  ADD_EQ,
  SUB_EQ,
  MUL_EQ,
  DIV_EQ,
  INCR,
  DECR,
  L_AND_EQ,
  L_OR_EQ,
  B_AND_EQ,
  B_OR_EQ,
  B_XOR_EQ,
  POW_EQ,
  REM_EQ,
  R_SHIFT_EQ,
  L_SHIFT_EQ,

  // KEYWORD
  AS,
  ASYNC,
  AWAIT,
  CASE,
  CLASS,
  DEFAULT,
  DO,
  ELSE,
  FALSE,
  FOR,
  FROM,
  FX,
  IF,
  IMPORT,
  NEW,
  RETURN,
  SELECT,
  SWITCH,
  TRUE,
  WHILE,
  NUL,

  // TYPE
  STR,       // "hello world"
  NAME,      // var_name
  DEC,       // 123
  HEX,       // 0xff
  OCTAL,     // 0o77
  BINARY,    // 0b1111
  DOT,       // .
  SEMICOLON, // ;
  NEWLINE,   // \n todo remove
  COMMA,     // ,
  AT,        // @
  HASH,      // #
  DOLLAR,    // $
  QUERY,     // ?
  L_BRACKET, // [
  R_BRACKET, // ]
  L_PAREN,   // (
  R_PAREN,   // )
  L_CURLY,   // {
  R_CURLY,   // }
  COLON,     // :
  END,       // \0

  // RANGE
  RANGE_EXCL, // ..
  RANGE_L_IN, // =..
  RANGE_R_IN, // ..=
  RANGE_INCL, // =..=

  // OPERATION
  ADD,     // +
  SUB,     // -
  MUL,     // *
  DIV,     // /
  L_AND,   // &&
  L_OR,    // ||
  B_AND,   // &
  B_OR,    // |
  CARET,   // ^
  B_NOT,   // ~ (unary)
  L_NOT,   // ! (unary)
  L_SHIFT, // <<
  R_SHIFT, // >>
  POW,     // **
  REM,     // %

  MAX_TOKENS
} Type;

typedef union Any {
  char               i8;
  short              i16;
  int                i32;
  long long          i64;
  unsigned char      u8;
  unsigned short     u16;
  unsigned int       u32;
  unsigned long long u64;
  float              f32;
  double             f64;
  void              *ptr;
  const char        *str;
} Any;

typedef struct Token {
  Type type;
  // todo temp
  uint8_t index{};

  struct Position {
    int line;
    int start;
    int end;
    int absStart;
    int absEnd;
  } pos;
  Any value;
} Token;

#endif // HADRON_TYPES_H
