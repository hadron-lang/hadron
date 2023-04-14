#include "c.h"

static void compile(Typed *v, small indent, FILE *fp);
static void root_compile(Program *p, FILE *fp);
static void tab(small i, FILE *fp);
static char *getBinaryOperator(BinaryOperator);

void tab(small i, FILE *fp) {
	for (small j = 0; j < i; j++) {
		fputs("\t", fp);
	}
}

char *getBinaryOperator(BinaryOperator oper) {
	switch (oper) {
		case BIN_ADD: return "+";
		case BIN_SUB: return "-";
		case BIN_MUL: return "*";
		case BIN_DIV: return "/";
		case BIN_LAND: return "&&";
		case BIN_LOR: return "||";
		case BIN_BAND: return "&";
		case BIN_BOR: return "|";
		case BIN_BXOR: return "^";
		case BIN_REM: return "%";
		case BIN_RSHIFT: return ">>";
		case BIN_LSHIFT: return "<<";
		case BIN_POW: return "**";
		default: return "";
	}
}

void compile(Typed *v, small indent, FILE *fp) {
	switch(v->type) {
		case BINARY_EXPRESSION: {
			BinaryExpression *expr = (BinaryExpression *)v;
			// tab(indent);
			compile((Typed *)expr->left, indent+1, fp);
			fputc(' ', fp);
			fputs(getBinaryOperator(expr->operator), fp);
			fputc(' ', fp);
			compile((Typed *)expr->right, indent+1, fp);
			// printf(";\n");
			break;
		}
		case CALL_EXPRESSION: {
			CallExpression *expr = (CallExpression *)v;
			tab(indent, fp);
			compile((Typed *)expr->callee, indent+1, fp);
			fputs("(", fp);
			for (int i = 0; i < expr->params->l; i++) {
				compile((Typed *)expr->params->a[i], indent+1, fp);
			}
			fputs(")", fp);
			fputs(";\n", fp);
			break;
		}
		case IDENTIFIER: {
			Identifier *id = (Identifier *)v;
			if (!strcmp(id->name, "log")) {
				fputs("eeefddfb05b0594dd3a6e5198aa7fa46", fp);
			} else fputs(id->name, fp);
			break;
		}
		case LITERAL: {
			Literal *lit = (Literal *)v;
			fputs(lit->value, fp);
			break;
		}
		default: break;
	}
}
void root_compile(Program *p, FILE *fp) {
	Array *r = newArray(p->body->l/2);
	Array *e = newArray(p->body->l/2);
	for (int i = 0; i < p->body->l; i++) {
		Typed *v = p->body->a[i];
		if (
			v->type == FUNCTION_DECLARATION ||
			v->type == IMPORT_DECLARATION ||
			v->type == CLASS_DECLARATION
		) { pushArray(r, v); continue; }
		pushArray(e, v);
	}
	for (int i = 0; i < r->l; i++) {
		compile(r->a[i], 0, fp);
	}
	fputs("#include \"../src/compilers/lib/std.h\"\n\n", fp);
	fputs("int main(int argc, char **argv) {\n", fp);
	for (int i = 0; i < e->l; i++) {
		compile(e->a[i], 1, fp);
	}
	fputs("}\n", fp);
}

Result *c_compile(Program *prog) {
	Result *res = malloc(sizeof(Result));
	res->errors = newArray(2);

	FILE *fp = fopen("tests/out.c", "w");

	root_compile(prog, fp);

	fclose(fp);

	res->data = NULL;
	return res;
}
