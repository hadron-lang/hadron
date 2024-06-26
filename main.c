#include "main.h"

static boolean check(Array *, boolean);
static boolean isLang(char *);
static boolean isFile(char *);
static Lang    parseLang(char *);
static void    init(int, char *[], Array *);

static struct Args {
  Lang    lang;
  Mode    mode;
  boolean set;
  char   *file;
} args;

void init(int argc, char *argv[], Array *errs) {
  args.lang = L_UNKN;
  args.mode = INTERPRET;
  args.set  = false;
  args.file = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0) {
      if (args.set && args.mode == INTERPRET)
        pushArray(errs, clierror(3, "\x1b[96m--interpret",
                          " and \x1b[96m--compile", " are mutually exclusive"));
      args.mode = COMPILE;
      args.set  = true;
    } else if (strcmp(argv[i], "-i") == 0 ||
               strcmp(argv[i], "--interpret") == 0) {
      if (args.lang)
        pushArray(
          errs, clierror(1, "language is not configurable in interpreter"));
      if (args.set && args.mode == COMPILE)
        pushArray(errs, clierror(3, "\x1b[96m--interpret",
                          " and \x1b[96m--compile", " are mutually exclusive"));
      args.mode = INTERPRET;
      args.set  = true;
    } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lang") == 0) {
      if (isLang(argv[i + 1])) args.lang = parseLang(argv[++i]);
    } else if (isFile(argv[i]))
      args.file = argv[i];
  };
  if (args.mode == INTERPRET && args.lang)
    pushArray(errs, clierror(1, "language is not available in interpreter"));
  if (args.mode == COMPILE && !args.file)
    pushArray(
      errs, clierror(1, "you should provide at least one file to compile"));
  if (args.mode == COMPILE && !args.lang)
    pushArray(
      errs, clierror(1, "you should choose one language to compile to"));
  if (args.mode == INTERPRET && !args.file)
    pushArray(errs, clierror(1, "REPL interpret mode not available yet"));
}

int main(int argc, char *argv[]) {
  Array *errors = newArray(1);
  init(argc, argv, errors);

  if (check(errors, false)) return -1;

  FILE *fp = fopen(args.file, "rb");
  if (fp == NULL) {
    char *e = utfcat("file \x1b[97m", args.file);
    pushArray(errors, clierror(2, e, "\x1b[0;91m does not exist"));
    free(e);
  }
  if (check(errors, false)) return -1;

  fseek(fp, 0, SEEK_END);
  size_t fsize = ftell(fp);
  rewind(fp);

  char *contents = malloc(fsize + 1);

  if (contents == NULL)
    pushArray(errors, clierror(1, "could not allocate enough space for file"));
  if (check(errors, false)) return -1;

  size_t read = fread(contents, 1UL, fsize, fp);
  if (read < fsize)
    pushArray(errors, clierror(1, "could not read file entirely"));
  if (check(errors, true)) return -1;

  contents[fsize] = 0;

  fclose(fp);

  Result *t = tokenize(contents, args.file);
  if (check(t->errors, true)) return -1;

  // printTokens(contents, t->data);

  Result *p = parse(contents, t->data, args.file);
  // if (check(p->errors, true)) return -1;

  check(p->errors, true);

  Result *c = compile(contents, p->data, args.file);

  printAST(p->data, 10);

  printf("token count %i\n", ((Array *)t->data)->length);

  free(contents);
  freeArray(t->data);
  free(t);
  freeArray(((Program *)p->data)->body);
  free(p);
}

boolean check(Array *errors, boolean drop) {
  if (errors->length) {
    printErrors(errors);
    if (drop) freeArray(errors);
    return 1;
  };
  if (drop) freeArray(errors);
  return 0;
}

boolean isLang(char *arg) {
  for (size_t i = 0; i < strlen(arg); i++) {
    if (!((arg[i] >= 'a' && arg[i] <= 'z') ||
          (arg[i] >= 'A' && arg[i] <= 'Z') || arg[i] == '+'))
      return false;
  }
  return true;
}

boolean isFile(char *arg) {
  for (size_t i = 0; i < strlen(arg); i++) {
    if (!((arg[i] >= 'a' && arg[i] <= 'z') ||
          (arg[i] >= 'A' && arg[i] <= 'Z') ||
          (arg[i] >= '0' && arg[i] <= '9') || arg[i] == '+' || arg[i] == '.' ||
          arg[i] == '$' || arg[i] == '~' || arg[i] == '-' || arg[i] == '\\' ||
          arg[i] == '/' || arg[i] == '_'))
      return false;
  }
  return true;
}

Lang parseLang(char *arg) {
  if (strcasecmp(arg, "js") == 0 || strcasecmp(arg, "javascript") == 0) {
    return L_JS;
  } else if (strcasecmp(arg, "asm") == 0 || strcasecmp(arg, "assembly") == 0) {
    return L_ASM;
  } else if (strcasecmp(arg, "c") == 0) {
    return L_C;
  }
  return L_UNKN;
}
