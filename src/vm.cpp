#include "vm.hpp"
#include "logger.hpp"

#include <cmath>
#include <cstdio>

void print_stack(double stack[], int sp) {
	for (int i = 0; i <= sp; i++) {
		printf("[%g] ", stack[i]);
	}
	printf("\n");
}

InterpretResult VM::interpret(const Chunk &chunk) {
	ip = chunk.code.data();
	end = ip + chunk.code.size();
	sp = 0;

	try {
		while (ip < end) {
			uint8_t instruction = *ip++;
			switch (static_cast<OpCode>(instruction)) {
				case OpCode::FX_ENTRY:
				case OpCode::FX_EXIT:
					break;
				case OpCode::MOVE:
					if (ip + sizeof(double) > end)
						return InterpretResult::RUNTIME_ERROR;
					double operand;
					std::memcpy(&operand, ip, sizeof(double));
					ip += sizeof(double);
					push(operand);
					break;
				case OpCode::RETURN:
					printf("%g\n", pop());
					return InterpretResult::OK;
				case OpCode::ADD:
					push(pop() + pop());
					break;
				case OpCode::MUL:
					push(pop() * pop());
					break;
				case OpCode::SUB: {
					double a = pop();
					push(pop() - a);
					break;
				} case OpCode::DIV: {
					double a = pop();
					push(pop() / a);
					break;
				} case OpCode::REM: {
					double a = pop();
					push(fmod(pop(), a));
					break;
				} case OpCode::POW: {
					double exponent = pop();
					push(pow(pop(), exponent));
					break;
				}
				case OpCode::L_AND:
					push(pop() && pop());
					break;
				case OpCode::L_OR:
					push(pop() || pop());
					break;
				case OpCode::B_AND:
					push(static_cast<intmax_t>(pop()) & static_cast<intmax_t>(pop()));
					break;
				case OpCode::B_OR:
					push(static_cast<intmax_t>(pop()) | static_cast<intmax_t>(pop()));
					break;
				case OpCode::NEGATE:
					stack[sp - 1] = -stack[sp - 1];
					break;
				case OpCode::NOT:
					stack[sp - 1] = !static_cast<bool>(stack[sp - 1]);
					break;
				case OpCode::B_NOT:
					stack[sp] = ~static_cast<intmax_t>(stack[sp]);
					break;
				case OpCode::CMP_EQ:
					push(pop() == pop());
					break;
				case OpCode::CMP_NEQ:
					push(pop() != pop());
					break;
				case OpCode::RANGE_EXCL:
				case OpCode::RANGE_L_IN:
				case OpCode::RANGE_R_IN:
				case OpCode::RANGE_INCL:
					printf("Range [%2$g, %1$g]\n", pop(), pop());
					push(0);
					break;
				default:
					return InterpretResult::RUNTIME_ERROR;
			}
		}
	} catch (std::runtime_error err) {
		Logger::error(err.what());
	}
	return InterpretResult::RUNTIME_ERROR;
}
