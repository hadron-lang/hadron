#include "logger.h"
#include "types.h"
#include "error.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char *getData(const Token &t) {
  switch (t.type) {
    case Types::CMP_EQ:
      return "==";
    case Types::CMP_NEQ:
      return "!=";
    case Types::CMP_GT:
      return ">";
    case Types::CMP_GEQ:
      return ">=";
    case Types::CMP_LT:
      return "<";
    case Types::CMP_LEQ:
      return "<=";
    case Types::CST_EQ:
      return "#=";
    case Types::SET_EQ:
      return "$=";
    case Types::EQ:
      return "=";
    case Types::ADD_EQ:
      return "+=";
    case Types::SUB_EQ:
      return "-=";
    case Types::MUL_EQ:
      return "*=";
    case Types::DIV_EQ:
      return "/=";
    case Types::INCR:
      return "++";
    case Types::DECR:
      return "--";
    case Types::L_AND_EQ:
      return "&&=";
    case Types::L_OR_EQ:
      return "||=";
    case Types::B_AND_EQ:
      return "&=";
    case Types::B_OR_EQ:
      return "|=";
    case Types::B_XOR_EQ:
      return "^=";
    case Types::POW_EQ:
      return "**=";
    case Types::REM_EQ:
      return "%=";
    case Types::R_SHIFT_EQ:
      return ">>=";
    case Types::L_SHIFT_EQ:
      return "<<=";
    case Types::AS:
      return "as";
    case Types::ASYNC:
      return "async";
    case Types::AWAIT:
      return "await";
    case Types::CASE:
      return "case";
    case Types::CLASS:
      return "class";
    case Types::DEFAULT:
      return "default";
    case Types::DO:
      return "do";
    case Types::ELSE:
      return "else";
    case Types::FALSE:
      return "false";
    case Types::FOR:
      return "for";
    case Types::FROM:
      return "from";
    case Types::FX:
      return "fx";
    case Types::IF:
      return "if";
    case Types::IMPORT:
      return "import";
    case Types::NEW:
      return "new";
    case Types::RETURN:
      return "return";
    case Types::SELECT:
      return "select";
    case Types::SWITCH:
      return "switch";
    case Types::TRUE:
      return "true";
    case Types::WHILE:
      return "while";
    case Types::NUL:
      return "null";
    case Types::STR:
      return "STR";
    case Types::NAME:
      return "NAME";
    case Types::DEC:
      return "NUM";
    case Types::DOT:
      return ".";
    case Types::RANGE_EXCL:
      return "..";
    case Types::RANGE_R_IN:
      return "..=";
    case Types::RANGE_L_IN:
      return "=..";
    case Types::RANGE_INCL:
      return "=..=";
    case Types::SEMICOLON:
      return ";";
    case Types::NEWLINE:
      return "\\n";
    case Types::COMMA:
      return ",";
    case Types::AT:
      return "@";
    case Types::HASH:
      return "#";
    case Types::DOLLAR:
      return "$";
    case Types::QUERY:
      return "?";
    case Types::L_BRACKET:
      return "[";
    case Types::R_BRACKET:
      return "]";
    case Types::L_PAREN:
      return "(";
    case Types::R_PAREN:
      return ")";
    case Types::L_CURLY:
      return "{";
    case Types::R_CURLY:
      return "}";
    case Types::HEX:
      return "HEX";
    case Types::OCTAL:
      return "OCT";
    case Types::BINARY:
      return "BIN";
    case Types::COLON:
      return ":";
    case Types::END:
      return "\\0";
    case Types::ADD:
      return "+";
    case Types::SUB:
      return "-";
    case Types::MUL:
      return "*";
    case Types::DIV:
      return "/";
    case Types::L_AND:
      return "&&";
    case Types::L_OR:
      return "||";
    case Types::B_AND:
      return "&";
    case Types::B_OR:
      return "|";
    case Types::CARET:
      return "^";
    case Types::B_NOT:
      return "~";
    case Types::L_NOT:
      return "!";
    case Types::L_SHIFT:
      return "<<";
    case Types::R_SHIFT:
      return ">>";
    case Types::POW:
      return "**";
    case Types::REM:
      return "%";
    case Types::MAX_TOKENS:
      return "MAX";
    case Types::ERROR:
      return "ERROR";
  }
  return "";
}

static bool supportsAnsi() {
  static bool checked   = false;
  static bool supported = false;

  if (checked)
    return supported;
  checked = true;
  if (const char *term = secure_getenv("TERM");
    term && (strstr(term, "xterm") || strstr(term, "color")))
    supported = true;
  return supported;
}

void Logger::info(const char *msg) {
  if (supportsAnsi())
    printf("\x1b[96mINFO\x1b[m %s\n", msg);
  else
    printf("INFO: %s\n", msg);
}

void Logger::warn(const char *msg) {
  if (supportsAnsi())
    printf("\x1b[93mWARN\x1b[m %s\n", msg);
  else
    printf("WARN: %s\n", msg);
}

void Logger::error(const char *msg) {
  if (supportsAnsi())
    printf("\x1b[91mERROR\x1b[m %s\n", msg);
  else
    printf("ERROR: %s\n", msg);
}

void Logger::fatal(const char *msg) {
  if (supportsAnsi())
    printf("\x1b[91;7;1m FATAL \x1b[0;91m %s\x1b[m\n", msg);
  else
    printf("FATAL: %s\n", msg);
  exit(EXIT_FAILURE);
}

void Logger::fatal(const char *msg, Token *tkn, const char *filename) {
  Error err = create_error((char *)"Syntax", filename, (char *)msg, tkn);
  print_error(&err);
}

void Logger::print_token(const Token &token) {
  const bool ansi   = supportsAnsi();
  const auto key0   = ansi ? "\x1b[94m" : "";
  const auto key1   = ansi ? "\x1b[96m" : "";
  const auto text   = ansi ? "\x1b[92m" : "";
  const auto value  = ansi ? "\x1b[93m" : "";
  const auto clear  = ansi ? "\x1b[m" : "";
  const auto italic = ansi ? "\x1b[3m" : "";
  const auto grey   = ansi ? "\x1b[37m" : "";
  printf("%s%sToken%s ", italic, grey, clear);
  printf("<%s%s%s> ", text, getData(token), clear);
  printf("{%s type%s: ", key0, clear);
  printf("%s%i%s,%s pos%s: {%s line%s: ", value,
    static_cast<uint8_t>(token.type), clear, key0, clear, key1, clear);
  printf("%s%i%s,%s start%s: ", value, token.pos.line, clear, key1, clear);
  printf("%s%i%s,%s end%s: ", value, token.pos.start, clear, key1, clear);
  printf("%s%i%s,%s absStart%s: ", value, token.pos.end, clear, key1, clear);
  printf("%s%i%s,%s absEnd%s: ", value, token.pos.absStart, clear, key1, clear);
  printf("%s%i%s }\n", value, token.pos.absEnd, clear);
}

static void print_bytes(
  const int bytes, const Chunk &chunk, int *offset, const char *desc) {
  for (int i = 0; i < bytes; i++) {
    printf("%02x ", chunk.code[(*offset)++]);
  }
  for (int i = bytes; i < 10; i++) {
    printf("   ");
  }
  printf("%s\n", desc);
}

void Logger::disassemble(const Chunk &chunk, const char *name) {
  printf("=== %s ===\n", name);

  for (int offset = 0; offset < chunk.pos;) {
    printf(" %04x: ", offset);

    switch (static_cast<OpCode>(chunk.code[offset])) {
      case OpCodes::RETURN:
        print_bytes(1, chunk, &offset, "RET");
        break;
      case OpCodes::ADD:
        print_bytes(1, chunk, &offset, "ADD");
        break;
      case OpCodes::SUB:
        print_bytes(1, chunk, &offset, "SUB");
        break;
      case OpCodes::MUL:
        print_bytes(1, chunk, &offset, "MUL");
        break;
      case OpCodes::DIV:
        print_bytes(1, chunk, &offset, "DIV");
        break;
      case OpCodes::POW:
        print_bytes(1, chunk, &offset, "POW");
        break;
      case OpCodes::NEGATE:
        print_bytes(1, chunk, &offset, "NEG");
        break;
      case OpCodes::NOT:
        print_bytes(1, chunk, &offset, "NOT");
        break;
      case OpCodes::B_NOT:
        print_bytes(1, chunk, &offset, "B_NOT");
        break;
      case OpCodes::MOVE:
        print_bytes(10, chunk, &offset, "MOVE");
        break;
      case OpCodes::LOAD:
        print_bytes(2, chunk, &offset, "LOAD");
        break;
      case OpCodes::STORE:
        print_bytes(2, chunk, &offset, "STORE");
        break;
      case OpCodes::RANGE_EXCL:
      case OpCodes::RANGE_L_IN:
      case OpCodes::RANGE_R_IN:
      case OpCodes::RANGE_INCL:
        print_bytes(1, chunk, &offset, "RANGE");
        break;
      case OpCodes::FX_ENTRY:
        print_bytes(1, chunk, &offset, "FX ENTRY");
        break;
      case OpCodes::FX_EXIT:
        print_bytes(1, chunk, &offset, "FX EXIT");
        break;
      default:
        print_bytes(1, chunk, &offset, "UNKNOWN");
    }
  }
}
