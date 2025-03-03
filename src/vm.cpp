#include "vm.h"
#include "logger.h"

#include <cmath>
#include <cstdio>

void print_stack(double stack[], int sp) {
  for (int i = 0; i <= sp; i++) {
    printf("[%g] ", stack[i]);
  }
  printf("\n");
}

InterpretResult VM::interpret(Chunk &chunk) {
  for (int ip = 0; ip < chunk.pos; ip++) {
    // print_stack(stack, sp);
    switch (const auto opcode = static_cast<OpCode>(chunk.code[ip]); opcode) {
      case OpCodes::FX_ENTRY:
      case OpCodes::FX_EXIT:
        break;
      case OpCodes::MOVE:
        stack[++sp] = *reinterpret_cast<double *>(chunk.code + ip + 2);
        ip += 9;
        break;
      case OpCodes::PRINT:
        printf("%g\n", stack[sp--]);
        break;
      case OpCodes::RETURN:
        return INTERPRET_OK;
      case OpCodes::ADD:
        stack[sp - 1] = stack[sp - 1] + stack[sp];
        sp--;
        break;
      case OpCodes::MUL:
        stack[sp - 1] = stack[sp - 1] * stack[sp];
        sp--;
        break;
      case OpCodes::SUB:
        stack[sp - 1] = stack[sp - 1] - stack[sp];
        sp--;
        break;
      case OpCodes::DIV:
        stack[sp - 1] = stack[sp - 1] / stack[sp];
        sp--;
        break;
      case OpCodes::POW:
        stack[sp - 1] = pow(stack[sp - 1], stack[sp]);
        sp--;
        break;
      case OpCodes::L_AND:
        stack[sp - 1] = stack[sp - 1] && stack[sp];
        sp--;
        break;
      case OpCodes::L_OR:
        stack[sp - 1] = stack[sp - 1] || stack[sp];
        sp--;
        break;
      case OpCodes::B_AND:
        // stack[sp - 1] = stack[sp - 1] & stack[sp];
        // sp--;
        Logger::fatal("& can only be applied to integers");
        break;
      case OpCodes::NEGATE:
        stack[sp] = -stack[sp];
        break;
      case OpCodes::NOT:
        stack[sp] = !static_cast<bool>(stack[sp]);
        break;
      case OpCodes::B_NOT:
        // stack[sp] = ~stack[sp];
        Logger::fatal("~ can only be applied to integers");
        break;
      case OpCodes::RANGE_EXCL:
      case OpCodes::RANGE_L_IN:
      case OpCodes::RANGE_R_IN:
      case OpCodes::RANGE_INCL:
        printf("Range [%g, %g]\n", stack[sp - 1], stack[sp]);
        stack[--sp] = 0;
        break;
      default:
        Logger::fatal("Unknown opcode\n");
    }
  }
  return INTERPRET_RUNTIME_ERROR;
}
