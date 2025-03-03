#include "arguments.h"
#include "file.h"
#include "lexer.h"
#include "parser.h"
#include "logger.h"
#include "vm.h"

#include <cstring>

#define MAX_EXT_LENGTH      0x10
#define MAX_DIR_LENGTH      0x100
#define MAX_FILENAME_LENGTH 0x100

static void init_arguments(
  ArgumentParser *parser, const int argc, char *argv[]) {
  parser->add("verbose", 'v', false);
  parser->add("lang", 'l');
  parser->add("out", 'o');
  parser->add("disassemble", 'd', false);
  //! deprecated options
  parser->add("compile", 'c', false);
  parser->add("interpret", 'i', false);

  parser->parse(argc, argv);
}

// todo remove temp function
void build_path(const File &file, char *path) {
  char dir[MAX_DIR_LENGTH];
  char name[MAX_FILENAME_LENGTH];

  file.get_dir(dir);   // No allocations in get_dir
  file.get_name(name); // No allocations in get_name

  snprintf(
    path, MAX_DIR_LENGTH + MAX_FILENAME_LENGTH + 6, "%s/%s.hbc", dir, name);
}

static void repl() {
  Chunk  chunk;
  VM     vm;
  Input  input("");
  Lexer  lexer(input);
  Parser parser(lexer, chunk);

  for (;;) {
    char line[0x400];
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    input = Input(line);

    lexer.reset(input);
    chunk.clear();
    parser.parse();
    vm.interpret(chunk);
  }
}

int main(const int argc, char *argv[]) {
  ArgumentParser argument_parser;

  init_arguments(&argument_parser, argc, argv);

  if (argument_parser.positional.empty()) {
    repl();
  }

  for (const auto filename : argument_parser.positional) {
    File  file(filename, FILE_MODE_READ);
    Input input(file);
    Chunk chunk;

    char ext[MAX_EXT_LENGTH];
    file.get_ext(ext);

    if (strncmp(ext, "hbc", 3) == 0) {
      FileHeader header;
      if (file.read_header(&header) != FILE_STATUS_OK) {
        Logger::fatal("Failed to read header");
      }
      char name[MAX_FILENAME_LENGTH];
      if (file.read_name(name, header.name)) {
        Logger::fatal("Failed to read name");
      }

      char c;
      while (file.read_byte(&c) != FILE_READ_DONE) {
        chunk.write(c);
      }
      if (argument_parser.is_set("disassemble")) {
        Logger::disassemble(chunk, name);
        continue;
      }

      VM().interpret(chunk);
      continue;
    }

    Lexer lexer(input);
    Parser parser(lexer, chunk);

    parser.parse();

    char path[MAX_DIR_LENGTH + MAX_FILENAME_LENGTH];
    build_path(file, path);

    File out(path, FILE_MODE_WRITE);

    if (out.write_header() != FILE_STATUS_OK) {
      Logger::fatal("Failed to write header");
    }

    char name[MAX_FILENAME_LENGTH];
    file.get_name(name);

    out << name;

    for (int i = 0; i < chunk.pos; i++) {
      out << chunk.code[i];
    }
    // Logger::disassemble(chunk, name);
  }

  return 0;
}
