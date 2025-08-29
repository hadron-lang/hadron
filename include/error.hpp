#ifndef HADRON_TEST_H
#define HADRON_TEST_H 1

#include "types.hpp"

typedef struct Error {
  const char  *name;
  const char  *file;
  const char  *data;
  const Token *token;
} Error;

Error create_error(const char *name, const char *file, const char *data, const Token *token);
void  print_error(const Error *err);

#endif
