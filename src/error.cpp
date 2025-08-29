#include "error.hpp"
#include "types.hpp"

#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

#define COLOR_RESET  "\x1b[0m"
#define COLOR_BOLD   "\x1b[1m"
#define COLOR_GRAY   "\x1b[90m"
#define COLOR_RED    "\x1b[91m"
#define COLOR_YELLOW "\x1b[93m"
#define COLOR_BLUE   "\x1b[94m"

char *substr(const char *s, const int start, const int len) {
  char *sub = static_cast<char *>(malloc(len + 1));
  memcpy(sub, s + start, len);
  sub[len] = 0;
  return sub;
}

int int_len(int x) {
  int len = 0;
  if (x <= 0)
    len++;
  while (x) {
    len++;
    x /= 10;
  }
  return len;
}

void repeat(char *c, int count) {
  for (int i = 0; i < count; i++)
    fprintf(stderr, "%s", c);
}

Error create_error(const char *name, const char *file, const char *data, const Token *token) {
  Error err;
  err.name  = name;
  err.file  = file;
  err.data  = data;
  err.token = token;
  return err;
}

static void print_error_line(FILE *fp, const Token *t) {
  char  *line = nullptr;
  size_t len  = 0;
  int    idx  = 1;

  while (getline(&line, &len, fp) != -1) {
    if (idx == t->pos.line) {
      printf(" " COLOR_BOLD "%d │ " COLOR_RESET, idx);
      // Highlight error portion
      printf("%.*s", t->pos.start - 1, line);
      printf(COLOR_RED "%.*s" COLOR_RESET, t->pos.end - t->pos.start,
        line + t->pos.start - 1);
      printf("%s", line + t->pos.end - 1);
      free(line);
      break;
    }
    idx++;
  }
}

void print_error(const Error *err) {
  FILE *fp = fopen(err->file, "r");
  if (fp == nullptr)
    return;

  repeat((char *)" ", int_len(err->token->pos.line) + 2);
  fprintf(stderr, "┌─ " COLOR_BLUE "%s" COLOR_RESET ":" COLOR_YELLOW "%d" COLOR_RESET ":" COLOR_YELLOW "%d" COLOR_RESET, err->file, err->token->pos.line,
    err->token->pos.start);
  fprintf(stderr, COLOR_BOLD COLOR_RED " %s Error\n" COLOR_RESET, err->name);
  print_error_line(fp, err->token);
  repeat((char *)" ", int_len(err->token->pos.line) + 2);
  fprintf(stderr, "╵");
  repeat((char *)" ", err->token->pos.start);
  fprintf(stderr, COLOR_RED "^");
  repeat((char *)"~", err->token->pos.end - err->token->pos.start - 1);
  fprintf(stderr, COLOR_RED " %s\n\n" COLOR_RESET, err->data);
}
