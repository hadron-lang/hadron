#ifndef HADRON_SYMBOL_H
#define HADRON_SYMBOL_H 1

#include <cstddef>
#include <cstdint>

#define SYMBOL_NAME_LEN   0x20
#define SYMBOL_TABLE_SIZE 0x100

typedef enum class SymbolType : uint8_t {
  NUL,
  FUNCTION,
  I32,
  STR,
} SymbolType;

struct Symbol {
  SymbolType type{0};
  bool       in_use{false};
  int        location{0};
  char       name[SYMBOL_NAME_LEN]{};
};

class SymbolTable {
  Symbol table[SYMBOL_TABLE_SIZE]{};

  static size_t hash(const char *name);

  Symbol *get_entry(const char *name);

  public:
  bool insert(const char *name, int location, SymbolType type);

  Symbol *lookup(const char *name);
};

#endif // HADRON_SYMBOL_H
