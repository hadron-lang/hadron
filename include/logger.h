#ifndef HADRON_LOGGER_H
#define HADRON_LOGGER_H 1

#include "bytecode.h"
#include "types.h"

class Logger {
  public:
  static void info(const char *msg);
  static void warn(const char *msg);
  static void error(const char *msg);
  static void fatal(const char *msg);
  static void print_token(const Token &token);
  static void disassemble(const Chunk &chunk, const char *name);
};

#endif // HADRON_LOGGER_H
