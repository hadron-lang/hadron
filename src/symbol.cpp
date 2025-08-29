#include "symbol.hpp"

#include <cstdio>
#include <cstring>

size_t SymbolTable::hash(const char *name) {
  size_t hash = 5381;
  while (*name) {
    hash = (hash << 5) + hash + *name;
    ++name;
  }
  return hash % SYMBOL_TABLE_SIZE;
}

void print_symbol(Symbol *x) {
  Symbol s = *x;
  printf("Symbol <%p> { type: %hhu, in_use: %s, location: %i, name: \"%s\" }\n",
    static_cast<void *>(x), static_cast<uint8_t>(s.type),
    s.in_use ? "true" : "false", s.location, s.name);
}

Symbol *SymbolTable::get_entry(const char *name) {
  const size_t idx = hash(name);
  for (size_t i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
    const size_t current_idx = (idx + i) % SYMBOL_TABLE_SIZE;
    Symbol      *entry       = &table[current_idx];
    if (!entry->in_use) {
      return entry;
    }
    if (strncmp(entry->name, name, SYMBOL_NAME_LEN) == 0) {
      return entry;
    }
  }
  return nullptr;
}

void print_table(Symbol *table) {
  printf("=== TABLE ===\n");
  for (size_t i = 0; i < SYMBOL_TABLE_SIZE; ++i) {
    printf("- ");
    print_symbol(&table[i]);
  }
  printf("=============\n");
}

bool SymbolTable::insert(
  const char *name, const int location, const SymbolType type) {
  Symbol *entry = get_entry(name);
  if (!entry)
    return false; // No empty bucket found
  if (strncmp(entry->name, name, SYMBOL_NAME_LEN) == 0)
    return false; // Already exists
  snprintf(entry->name, SYMBOL_NAME_LEN - 1, "%s", name);
  printf("-> \"%s\"\n", entry->name);
  entry->name[SYMBOL_NAME_LEN - 1] = '\0'; // Ensure null-termination
  entry->location                  = location;
  entry->type                      = type;
  entry->in_use                    = true;
  // print_table(table);
  return true;
}

Symbol *SymbolTable::lookup(const char *name) {
  Symbol *entry = get_entry(name);
  if (!entry)
    return nullptr; // Not found
  if (strncmp(entry->name, name, SYMBOL_NAME_LEN) == 0)
    return entry; // Found
  return nullptr; // Not found
}
