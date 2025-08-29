#ifndef HADRON_VM_H
#define HADRON_VM_H

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

enum class OpCode : uint8_t {
	NUL        = 0x00,
	RETURN     = 0x80,
	MOVE       = 0x81,
	ADD        = 0x90,
	SUB        = 0x91,
	MUL        = 0x92,
	DIV        = 0x93,
	POW        = 0x94,
	L_AND      = 0x95,
	L_OR       = 0x96,
	B_AND      = 0x97,
	B_OR       = 0x98,
	B_XOR      = 0x99,
	B_NOT      = 0x9a,
	NOT        = 0x9b,
	NEGATE     = 0x9c,
	REM        = 0x9d,
	LOAD       = 0xa0,
	STORE      = 0xa1,
	CMP_EQ     = 0xb0,
	CMP_NEQ    = 0xb1,
	CMP_GT     = 0xb2,
	CMP_GEQ    = 0xb3,
	CMP_LT     = 0xb4,
	CMP_LEQ    = 0xb5,
	FX_ENTRY   = 0xc0,
	FX_EXIT    = 0xc1,
	RANGE_EXCL = 0xd0,
	RANGE_L_IN = 0xd1,
	RANGE_R_IN = 0xd2,
	RANGE_INCL = 0xd3,
	PRINT      = 0xff,
};

class VM;

class Chunk {
public:
	void write_opcode(OpCode op) {
		code.push_back(static_cast<uint8_t>(op));
	}

	template <typename T>
	void write_operand(T operand) {
		const size_t old_size = code.size();
		code.resize(old_size + sizeof(T));
		std::memcpy(&code[old_size], &operand, sizeof(T));
	}

	size_t add_constant(double value) {
		constants.push_back(value);
		return constants.size() - 1;
	}

	void clear() {
		code.clear();
		constants.clear();
	}

	friend void print_bytes(int, const Chunk &, size_t *, const char *);
	friend class Logger;
	friend class File;
	friend class Parser;
	friend class VM;

	std::vector<uint8_t> code;
	std::vector<double> constants;
};

enum class InterpretResult {
	OK,
	COMPILE_ERROR,
	RUNTIME_ERROR
};

class VM {
public:
	VM() = default;

	InterpretResult interpret(const Chunk &chunk);

private:
	static constexpr size_t MAX_STACK = 0x1000;
	double stack[MAX_STACK] = {};
	size_t sp = 0;
	const uint8_t *ip = nullptr;
	const uint8_t *end = nullptr;

	void push(double value) {
		if (sp >= MAX_STACK)
			throw std::runtime_error("Stack overflow");
		stack[sp++] = value;
	}

	double pop() {
		if (sp == 0)
			throw std::runtime_error("Stack underflow");
		return stack[--sp];
	}
};

#endif // HADRON_VM_H
