#ifndef HADRON_VM_H
#define HADRON_VM_H

#include "util.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

typedef enum class OpCodes : uint8_t {
  RETURN     = 'r',
  PRINT      = '#',
  MOVE       = 'm',
  ADD        = '+',
  SUB        = '-',
  MUL        = '*',
  DIV        = '/',
  POW        = 'p',
  L_AND      = 'a',
  L_OR       = 'o',
  B_AND      = '&',
  B_OR       = '|',
  B_XOR      = '^',
  B_NOT      = '~',
  NOT        = '!',
  NEGATE     = 'n',
  LOAD       = 'l',
  STORE      = 's',
  RANGE_EXCL = 0x80,
  RANGE_L_IN = 0x81,
  RANGE_R_IN = 0x82,
  RANGE_INCL = 0x83,
  FX_ENTRY   = 0x90,
  FX_EXIT    = 0x91,
} OpCode;

#define MAX_INSTRUCTIONS 1024

class Chunk {
  public:
  int     pos{0};
  uint8_t code[MAX_INSTRUCTIONS]{};

  template <typename T> void write(T value) {
    if constexpr (std::is_same_v<T, char *>) {
      const size_t len = h_strlen(value);
      for (size_t i = 0; i < len; i++) {
        code[pos++] = value[i];
      }
    } else {
      *reinterpret_cast<T *>(code + pos) = value;
      pos += sizeof(T);
    }
  }
  void clear() { pos = 0; }
};

typedef enum InterpretResult {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} InterpretResult;

#define MAX_STACK     0x100
#define MAX_CONSTANTS 0x100

typedef class VM {
  // Chunk *chunk;
  // uint8_t *ip;
  double stack[MAX_STACK]{};
  // double constants[MAX_CONSTANTS]{};
  int sp{-1};
  // int pc{-1};

  public:
  VM() = default;

  InterpretResult interpret(Chunk &chunk);
} VM;

#endif // HADRON_VM_H
