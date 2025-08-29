#include "parser.hpp"
#include "types.hpp"
#include "vm.hpp"

Parser::Parser(Lexer &lexer, Chunk &chunk) : lexer(lexer), chunk(chunk) {}

void Parser::advance() {
	prev_token    = current_token;
	current_token = lexer.advance();
}

Token &Parser::consume(const Type type, const char *error) {
	if (current_token.type == type) {
		advance();
		return prev_token;
	}
	if (lexer.input.get_name())
		Logger::error(error, &current_token, lexer.input.get_name());
	else
		Logger::error(error, &current_token);
	return current_token;
}

bool Parser::match(const Type type) {
	if (current_token.type == type) {
		advance();
		return true;
	}
	return false;
}

ParseRule &get_rule(Type token_type);

void parse_fxn(Parser &p, const Token &) {
	const auto name = static_cast<const char *>(
		p.consume(Types::NAME, "Expected function name").value.ptr);

	// TODO: Accept parameters
	p.consume(Types::L_PAREN, "Expected '(' after function name");
	while (p.current_token.type != Types::R_PAREN)
		p.advance();
	if (!p.match(Types::R_PAREN)) {
		Logger::fatal("Expected ')' after function name", &p.current_token, p.lexer.input.get_name());
		return;
	}

	p.chunk.write_opcode(OpCode::FX_ENTRY);

	p.consume(Types::L_CURLY, "Expected '{' to start function body");
	const size_t start_address = p.chunk.code.size();
	while (!p.match(Types::R_CURLY) && p.current_token.type != Types::END) {
		p.parse_expression(Precedence::NUL);
	}
	if (p.current_token.type == Types::END)
		Logger::fatal("Unexpected end of file", &p.current_token, p.lexer.input.get_name());


	// const size_t end_address = chunk.pos;

	// Emit bytecode for function definition
	// chunk.write(static_cast<uint8_t>(parameters.size()));
	// for (const auto &param : parameters) {
	// chunk.write(param.c_str());
	// }
	p.chunk.write_opcode(OpCode::FX_EXIT);

	const bool insert_result = p.symbols.insert(
		name, static_cast<int>(start_address), SymbolType::FUNCTION);
	if (!insert_result)
		Logger::fatal("Out of free symbols");
};

void parse_lit(Parser &p, const Token &token) {
	switch (token.type) {
		case Types::DEC:
		case Types::HEX:
		case Types::OCTAL:
		case Types::BINARY:
			p.chunk.write_opcode(OpCode::MOVE);
			p.chunk.write_operand(token.value.f64);
			// p.chunk.write_operand(token.index);
			// p.chunk.add_constant(token.value.f64);
			break;
		case Types::STR:
			p.symbols.insert(token.value.str, 0, SymbolType::STR);
			break;
		default:
			Logger::fatal("Unknown literal");
	}
};

void parse_unr(Parser &p, const Token &token) {
	p.parse_expression(get_rule(token.type).precedence);
	switch (token.type) {
		case Types::ADD: // unary + does nothing
			break;
		case Types::SUB:
			p.chunk.write_opcode(OpCode::NEGATE);
			break;
		case Types::L_NOT:
			p.chunk.write_opcode(OpCode::NOT);
			break;
		case Types::B_NOT:
			p.chunk.write_opcode(OpCode::B_NOT);
			break;
		default:
			Logger::fatal("Unknown unary operator");
	}
};

void parse_grp(Parser &p, const Token &) {
	p.parse_expression(Precedence::NUL);
	p.consume(Types::R_PAREN, "Expected ')'");
};

void parse_bin(Parser &p, const Token &token) {
	p.parse_expression(get_rule(token.type).precedence);

	switch (token.type) {
		case Types::ADD:
			p.chunk.write_opcode(OpCode::ADD);
			break;
		case Types::SUB:
			p.chunk.write_opcode(OpCode::SUB);
			break;
		case Types::MUL:
			p.chunk.write_opcode(OpCode::MUL);
			break;
		case Types::DIV:
			p.chunk.write_opcode(OpCode::DIV);
			break;
		case Types::REM:
			p.chunk.write_opcode(OpCode::REM);
			break;
		case Types::L_AND:
			p.chunk.write_opcode(OpCode::L_AND);
			break;
		case Types::L_OR:
			p.chunk.write_opcode(OpCode::L_OR);
			break;
		case Types::B_AND:
			p.chunk.write_opcode(OpCode::B_AND);
			break;
		case Types::B_OR:
			p.chunk.write_opcode(OpCode::B_OR);
			break;
		case Types::CARET:
			p.chunk.write_opcode(OpCode::B_XOR);
			break;
		case Types::POW:
			p.chunk.write_opcode(OpCode::POW);
			break;
		case Types::CMP_EQ:
		case Types::CMP_LT:
			p.chunk.write_opcode(OpCode::NUL);
			break;
		default:
			Logger::fatal("Unknown binary operator");
	}
};

void parse_rng(Parser &p, const Token &token) {
	p.parse_expression(get_rule(token.type).precedence);
	switch (token.type) {
		case Types::RANGE_EXCL:
			p.chunk.write_opcode(OpCode::RANGE_EXCL);
			break;
		case Types::RANGE_L_IN:
			p.chunk.write_opcode(OpCode::RANGE_L_IN);
			break;
		case Types::RANGE_R_IN:
			p.chunk.write_opcode(OpCode::RANGE_R_IN);
			break;
		case Types::RANGE_INCL:
			p.chunk.write_opcode(OpCode::RANGE_INCL);
			break;
		default:
			Logger::fatal("Unknown range operator");
	}
};

void parse_dcl(Parser &p, const Token &token) {
	switch (token.type) {
		case Types::NAME: {
			switch (p.current_token.type) {
				case Types::NAME: {
					Token name = p.consume(Types::NAME, "Expected variable name");
					p.consume(Types::EQ, "Expected assignment");
					p.parse_expression(Precedence::NUL);
					p.chunk.write_opcode(OpCode::STORE);
					p.symbols.insert(name.value.str, 0, SymbolType::I32);
					break;
				} case Types::CMP_LT: {
					break;
				} default: {
					break;
				}
			}
			break;
		}
		case Types::COLON: {
			p.consume(Types::COLON, "Expected colon");
			p.parse_expression(Precedence::NUL);
			break;
		}
		case Types::L_PAREN: {
			p.consume(Types::L_PAREN, "Expected '('");
			p.parse_expression(Precedence::GRP);
			p.consume(Types::R_PAREN, "Expected ')'");
			break;
		}
		case Types::IF: {
			p.parse_expression(Precedence::NUL);
		}
		default: {
			printf("%i\n", static_cast<int>(p.current_token.type));
			Logger::fatal("Unknown declaration", &p.current_token, p.lexer.input.get_name());
		}
	}
};

void parse_err(Parser &p, const Token &token) {
	(void)token;
	(void)p;
	// printf("Got error");
};

#define I         static_cast<int>

static ParseRule rules[I(Types::MAX_TOKENS)] = {
	[I(Types::ERROR)]      = {Precedence::MAX, parse_err, parse_err},
	[I(Types::CMP_EQ)]     = {Precedence::EQT, nullptr, parse_bin},
	[I(Types::CMP_NEQ)]    = {Precedence::EQT, nullptr, parse_bin},
	[I(Types::CMP_GT)]     = {Precedence::CMP, nullptr, parse_bin},
	[I(Types::CMP_GEQ)]    = {Precedence::CMP, nullptr, parse_bin},
	[I(Types::CMP_LT)]     = {Precedence::CMP, nullptr, parse_bin},
	[I(Types::CMP_LEQ)]    = {Precedence::CMP, nullptr, parse_bin},
	[I(Types::CST_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::SET_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::EQ)]         = {Precedence::ASG, nullptr, nullptr},
	[I(Types::ADD_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::SUB_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::MUL_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::DIV_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::INCR)]       = {Precedence::UNR, nullptr, nullptr},
	[I(Types::DECR)]       = {Precedence::UNR, nullptr, nullptr},
	[I(Types::L_AND_EQ)]   = {Precedence::ASG, nullptr, nullptr},
	[I(Types::L_OR_EQ)]    = {Precedence::ASG, nullptr, nullptr},
	[I(Types::B_AND_EQ)]   = {Precedence::ASG, nullptr, nullptr},
	[I(Types::B_OR_EQ)]    = {Precedence::ASG, nullptr, nullptr},
	[I(Types::B_XOR_EQ)]   = {Precedence::ASG, nullptr, nullptr},
	[I(Types::POW_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::REM_EQ)]     = {Precedence::ASG, nullptr, nullptr},
	[I(Types::R_SHIFT_EQ)] = {Precedence::ASG, nullptr, nullptr},
	[I(Types::L_SHIFT_EQ)] = {Precedence::ASG, nullptr, nullptr},
	[I(Types::AS)]         = {Precedence::NUL, nullptr, nullptr},
	[I(Types::ASYNC)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::AWAIT)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::CASE)]       = {Precedence::NUL, nullptr, nullptr},
	[I(Types::CLASS)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::DEFAULT)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::DO)]         = {Precedence::NUL, nullptr, nullptr},
	[I(Types::ELSE)]       = {Precedence::NUL, nullptr, nullptr},
	[I(Types::FALSE)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::FOR)]        = {Precedence::NUL, nullptr, nullptr},
	[I(Types::FROM)]       = {Precedence::NUL, nullptr, nullptr},
	[I(Types::FX)]         = {Precedence::NUL, parse_fxn, nullptr},
	[I(Types::IF)]         = {Precedence::NUL, nullptr, nullptr},
	[I(Types::IMPORT)]     = {Precedence::NUL, nullptr, nullptr},
	[I(Types::NEW)]        = {Precedence::NUL, nullptr, nullptr},
	[I(Types::RETURN)]     = {Precedence::NUL, nullptr, nullptr},
	[I(Types::SELECT)]     = {Precedence::NUL, nullptr, nullptr},
	[I(Types::SWITCH)]     = {Precedence::NUL, nullptr, nullptr},
	[I(Types::TRUE)]       = {Precedence::NUL, nullptr, nullptr},
	[I(Types::WHILE)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::NUL)]        = {Precedence::NUL, nullptr, nullptr},
	[I(Types::STR)]        = {Precedence::LIT, parse_lit, nullptr},
	[I(Types::NAME)]       = {Precedence::LIT, parse_dcl, nullptr},
	[I(Types::DEC)]        = {Precedence::LIT, parse_lit, nullptr},
	[I(Types::HEX)]        = {Precedence::LIT, parse_lit, nullptr},
	[I(Types::OCTAL)]      = {Precedence::LIT, parse_lit, nullptr},
	[I(Types::BINARY)]     = {Precedence::LIT, parse_lit, nullptr},
	[I(Types::DOT)]        = {Precedence::NUL, nullptr, nullptr},
	[I(Types::SEMICOLON)]  = {Precedence::NUL, nullptr, nullptr},
	[I(Types::NEWLINE)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::COMMA)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::AT)]         = {Precedence::NUL, nullptr, nullptr},
	[I(Types::HASH)]       = {Precedence::NUL, nullptr, nullptr},
	[I(Types::DOLLAR)]     = {Precedence::NUL, nullptr, nullptr},
	[I(Types::QUERY)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::L_BRACKET)]  = {Precedence::NUL, nullptr, nullptr},
	[I(Types::R_BRACKET)]  = {Precedence::NUL, nullptr, nullptr},
	[I(Types::L_PAREN)]    = {Precedence::GRP, parse_grp, nullptr},
	[I(Types::R_PAREN)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::L_CURLY)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::R_CURLY)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::COLON)]      = {Precedence::NUL, nullptr, nullptr},
	[I(Types::END)]        = {Precedence::NUL, nullptr, nullptr},
	[I(Types::RANGE_EXCL)] = {Precedence::RNG, nullptr, parse_rng},
	[I(Types::RANGE_L_IN)] = {Precedence::RNG, nullptr, parse_rng},
	[I(Types::RANGE_R_IN)] = {Precedence::RNG, nullptr, parse_rng},
	[I(Types::RANGE_INCL)] = {Precedence::RNG, nullptr, parse_rng},
	[I(Types::ADD)]        = {Precedence::TRM, parse_unr, parse_bin},
	[I(Types::SUB)]        = {Precedence::TRM, parse_unr, parse_bin},
	[I(Types::MUL)]        = {Precedence::FCT, nullptr, parse_bin},
	[I(Types::DIV)]        = {Precedence::FCT, nullptr, parse_bin},
	[I(Types::L_AND)]      = {Precedence::LND, nullptr, parse_bin},
	[I(Types::L_OR)]       = {Precedence::LOR, nullptr, parse_bin},
	[I(Types::B_AND)]      = {Precedence::BND, nullptr, parse_bin},
	[I(Types::B_OR)]       = {Precedence::BOR, nullptr, parse_bin},
	[I(Types::CARET)]      = {Precedence::XOR, nullptr, parse_bin},
	[I(Types::B_NOT)]      = {Precedence::UNR, parse_unr, nullptr},
	[I(Types::L_NOT)]      = {Precedence::UNR, parse_unr, nullptr},
	[I(Types::L_SHIFT)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::R_SHIFT)]    = {Precedence::NUL, nullptr, nullptr},
	[I(Types::POW)]        = {Precedence::EXP, nullptr, parse_bin},
	[I(Types::REM)]        = {Precedence::FCT, nullptr, parse_bin},
};
#undef I

ParseRule &get_rule(const Type token_type) {
	return rules[static_cast<int>(token_type)];
}

void Parser::parse() {
	advance();
	while (current_token.type != Types::END) {
		parse_expression(Precedence::NUL);
		const bool is_stmt =
			match(Types::SEMICOLON) || current_token.pos.line != prev_token.pos.line;
		if (!is_stmt || current_token.type == Types::END)
			chunk.write_opcode(OpCode::RETURN);
	}
}

void Parser::parse_expression(const Precedence precedence) {
	const Token token = current_token;
	advance();

	ParseRule rule = get_rule(token.type);
	if (!rule.nud) {
		if (lexer.input.get_name())
			Logger::error("Unexpected token", &current_token, lexer.input.get_name());
		else
			Logger::error("unexpected token", &current_token);
		return;
	}

	rule.nud(*this, token);

	while (precedence < get_rule(current_token.type).precedence) {
		Token operator_token = current_token;
		if (operator_token.type == Types::END) {
			break;
		}
		bool same_line = token.pos.line == operator_token.pos.line;

		rule = get_rule(operator_token.type);
		if (!rule.led && same_line) {
			if (lexer.input.get_name())
				Logger::error("Unexpected operator", &current_token, lexer.input.get_name());
			else
				Logger::error("Unexpected operator", &current_token);
			return;
		} else if (!same_line) {
			return;
		}
		advance();

		rule.led(*this, operator_token);
	}
}
