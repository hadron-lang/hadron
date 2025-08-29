#include "logger.hpp"
#include "types.hpp"
#include "error.hpp"
#include "vm.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static const char *getData(const Token &t) {
	switch (t.type) {
		case Type::CMP_EQ:
			return "==";
		case Type::CMP_NEQ:
			return "!=";
		case Type::CMP_GT:
			return ">";
		case Type::CMP_GEQ:
			return ">=";
		case Type::CMP_LT:
			return "<";
		case Type::CMP_LEQ:
			return "<=";
		case Type::CST_EQ:
			return "#=";
		case Type::SET_EQ:
			return "$=";
		case Type::EQ:
			return "=";
		case Type::ADD_EQ:
			return "+=";
		case Type::SUB_EQ:
			return "-=";
		case Type::MUL_EQ:
			return "*=";
		case Type::DIV_EQ:
			return "/=";
		case Type::INCR:
			return "++";
		case Type::DECR:
			return "--";
		case Type::L_AND_EQ:
			return "&&=";
		case Type::L_OR_EQ:
			return "||=";
		case Type::B_AND_EQ:
			return "&=";
		case Type::B_OR_EQ:
			return "|=";
		case Type::B_XOR_EQ:
			return "^=";
		case Type::POW_EQ:
			return "**=";
		case Type::REM_EQ:
			return "%=";
		case Type::R_SHIFT_EQ:
			return ">>=";
		case Type::L_SHIFT_EQ:
			return "<<=";
		case Type::AS:
			return "as";
		case Type::ASYNC:
			return "async";
		case Type::AWAIT:
			return "await";
		case Type::CASE:
			return "case";
		case Type::CLASS:
			return "class";
		case Type::DEFAULT:
			return "default";
		case Type::DO:
			return "do";
		case Type::ELSE:
			return "else";
		case Type::FALSE:
			return "false";
		case Type::FOR:
			return "for";
		case Type::FROM:
			return "from";
		case Type::FX:
			return "fx";
		case Type::IF:
			return "if";
		case Type::IMPORT:
			return "import";
		case Type::NEW:
			return "new";
		case Type::RETURN:
			return "return";
		case Type::SELECT:
			return "select";
		case Type::SWITCH:
			return "switch";
		case Type::TRUE:
			return "true";
		case Type::WHILE:
			return "while";
		case Type::NUL:
			return "null";
		case Type::STR:
			return "STR";
		case Type::NAME:
			return "NAME";
		case Type::DEC:
			return "NUM";
		case Type::DOT:
			return ".";
		case Type::RANGE_EXCL:
			return "..";
		case Type::RANGE_R_IN:
			return "..=";
		case Type::RANGE_L_IN:
			return "=..";
		case Type::RANGE_INCL:
			return "=..=";
		case Type::SEMICOLON:
			return ";";
		case Type::NEWLINE:
			return "\\n";
		case Type::COMMA:
			return ",";
		case Type::AT:
			return "@";
		case Type::HASH:
			return "#";
		case Type::DOLLAR:
			return "$";
		case Type::QUERY:
			return "?";
		case Type::L_BRACKET:
			return "[";
		case Type::R_BRACKET:
			return "]";
		case Type::L_PAREN:
			return "(";
		case Type::R_PAREN:
			return ")";
		case Type::L_CURLY:
			return "{";
		case Type::R_CURLY:
			return "}";
		case Type::HEX:
			return "HEX";
		case Type::OCTAL:
			return "OCT";
		case Type::BINARY:
			return "BIN";
		case Type::COLON:
			return ":";
		case Type::END:
			return "\\0";
		case Type::ADD:
			return "+";
		case Type::SUB:
			return "-";
		case Type::MUL:
			return "*";
		case Type::DIV:
			return "/";
		case Type::L_AND:
			return "&&";
		case Type::L_OR:
			return "||";
		case Type::B_AND:
			return "&";
		case Type::B_OR:
			return "|";
		case Type::CARET:
			return "^";
		case Type::B_NOT:
			return "~";
		case Type::L_NOT:
			return "!";
		case Type::L_SHIFT:
			return "<<";
		case Type::R_SHIFT:
			return ">>";
		case Type::POW:
			return "**";
		case Type::REM:
			return "%";
		case Type::MAX_TOKENS:
			return "MAX";
		case Type::ERROR:
			return "ERROR";
	}
	return "";
}

static bool supportsAnsi() {
	static bool checked   = false;
	static bool supported = false;

	if (checked)
		return supported;
	checked = true;
	if (const char *term = secure_getenv("TERM");
		term && (strstr(term, "xterm") || strstr(term, "color")))
		supported = true;
	return supported;
}

void Logger::info(const char *msg) {
	if (supportsAnsi())
		printf("\x1b[96mINFO\x1b[m %s\n", msg);
	else
		printf("INFO: %s\n", msg);
}

void Logger::warn(const char *msg) {
	if (supportsAnsi())
		printf("\x1b[93mWARN\x1b[m %s\n", msg);
	else
		printf("WARN: %s\n", msg);
}

void Logger::error(const char *msg) {
	if (supportsAnsi())
		printf("\x1b[91mERROR\x1b[m %s\n", msg);
	else
		printf("ERROR: %s\n", msg);
}

void Logger::fatal(const char *msg) {
	if (supportsAnsi())
		printf("\x1b[91;7;1m FATAL \x1b[0;91m %s\x1b[m\n", msg);
	else
		printf("FATAL: %s\n", msg);
	exit(EXIT_FAILURE);
}

#define COLOR_RESET  "\x1b[0m"
#define COLOR_BOLD   "\x1b[1m"
#define COLOR_GRAY   "\x1b[90m"
#define COLOR_RED    "\x1b[91m"
#define COLOR_YELLOW "\x1b[93m"
#define COLOR_BLUE   "\x1b[94m"

void Logger::error(const char *msg, Token *tkn) {
	fprintf(stderr, COLOR_BOLD COLOR_RED "Syntax Error at " COLOR_RESET COLOR_YELLOW "%d" COLOR_RESET ":" COLOR_YELLOW "%d\n" COLOR_RESET, tkn->pos.line,
		tkn->pos.start);
	fprintf(stderr, COLOR_RED "  %s\n" COLOR_RESET, msg);
}

void Logger::error(const char *msg, Token *tkn, const char *filename) {
	Error err = create_error((char *)"Syntax", filename, (char *)msg, tkn);
	print_error(&err);
}

void Logger::fatal(const char *msg, Token *tkn) {
	fprintf(stderr, COLOR_BOLD COLOR_RED "Syntax Error at " COLOR_RESET COLOR_YELLOW "%d" COLOR_RESET ":" COLOR_YELLOW "%d\n" COLOR_RESET, tkn->pos.line,
		tkn->pos.start);
	fprintf(stderr, COLOR_RED "  %s\n" COLOR_RESET, msg);
	exit(EXIT_FAILURE);
}

void Logger::fatal(const char *msg, Token *tkn, const char *filename) {
	Error err = create_error((char *)"Syntax", filename, (char *)msg, tkn);
	print_error(&err);
	exit(EXIT_FAILURE);
}

void Logger::print_token(const Token &token) {
	const bool ansi   = supportsAnsi();
	const auto key0   = ansi ? "\x1b[94m" : "";
	const auto key1   = ansi ? "\x1b[96m" : "";
	const auto text   = ansi ? "\x1b[92m" : "";
	const auto value  = ansi ? "\x1b[93m" : "";
	const auto clear  = ansi ? "\x1b[m" : "";
	const auto italic = ansi ? "\x1b[3m" : "";
	const auto grey   = ansi ? "\x1b[37m" : "";
	printf("%s%sToken%s ", italic, grey, clear);
	printf("<%s%s%s> ", text, getData(token), clear);
	printf("{%s type%s: ", key0, clear);
	printf("%s%i%s,%s pos%s: {%s line%s: ", value,
		static_cast<uint8_t>(token.type), clear, key0, clear, key1, clear);
	printf("%s%i%s,%s start%s: ", value, token.pos.line, clear, key1, clear);
	printf("%s%i%s,%s end%s: ", value, token.pos.start, clear, key1, clear);
	printf("%s%i%s,%s absStart%s: ", value, token.pos.end, clear, key1, clear);
	printf("%s%i%s,%s absEnd%s: ", value, token.pos.absStart, clear, key1, clear);
	printf("%s%i%s }\n", value, token.pos.absEnd, clear);

	if (token.type == Type::HEX)
		printf("[DBG] %f\n", token.value.f64);
}

void print_bytes(
	const int bytes, const Chunk &chunk, size_t *offset, const char *desc) {
	for (int i = 0; i < bytes; i++) {
		printf("%02x ", chunk.code.at((*offset)++));
	}
	for (int i = bytes; i < 10; i++) {
		printf("   ");
	}
	printf("%s\n", desc);
}

void Logger::disassemble(const Chunk &chunk, const char *name) {
	printf("=== %s ===\n", name);

	for (size_t offset = 0; offset < chunk.code.size();) {
		printf(" %04lx: ", offset);

		switch (static_cast<OpCode>(chunk.code.at(offset))) {
			case OpCode::NUL:
				print_bytes(1, chunk, &offset, "NUL");
				break;
			case OpCode::RETURN:
				print_bytes(1, chunk, &offset, "RET");
				break;
			case OpCode::ADD:
				print_bytes(1, chunk, &offset, "ADD");
				break;
			case OpCode::SUB:
				print_bytes(1, chunk, &offset, "SUB");
				break;
			case OpCode::MUL:
				print_bytes(1, chunk, &offset, "MUL");
				break;
			case OpCode::DIV:
				print_bytes(1, chunk, &offset, "DIV");
				break;
			case OpCode::POW:
				print_bytes(1, chunk, &offset, "POW");
				break;
			case OpCode::REM:
				print_bytes(1, chunk, &offset, "REM");
				break;
			case OpCode::NEGATE:
				print_bytes(1, chunk, &offset, "NEG");
				break;
			case OpCode::NOT:
				print_bytes(1, chunk, &offset, "NOT");
				break;
			case OpCode::B_NOT:
				print_bytes(1, chunk, &offset, "B_NOT");
				break;
			case OpCode::MOVE:
				print_bytes(9, chunk, &offset, "MOVE");
				break;
			case OpCode::LOAD:
				print_bytes(2, chunk, &offset, "LOAD");
				break;
			case OpCode::STORE:
				print_bytes(2, chunk, &offset, "STORE");
				break;
			case OpCode::RANGE_EXCL:
			case OpCode::RANGE_L_IN:
			case OpCode::RANGE_R_IN:
			case OpCode::RANGE_INCL:
				print_bytes(1, chunk, &offset, "RANGE");
				break;
			case OpCode::FX_ENTRY:
				print_bytes(1, chunk, &offset, "FX ENTRY");
				break;
			case OpCode::FX_EXIT:
				print_bytes(1, chunk, &offset, "FX EXIT");
				break;
			default:
				print_bytes(1, chunk, &offset, "UNKNOWN");
		}
	}
}
