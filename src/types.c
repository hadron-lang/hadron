#include "types.h"

char *getTokenContent(char *src, Token *t) {
	boolean str = t->type == STR;
	int		l	= t->pos.absEnd - t->pos.absStart - (str ? 2 : 0);
	char   *sub = (char *)malloc(l + 1);
	memcpy(sub, src + t->pos.absStart + (str ? 1 : 0), l);
	sub[l] = 0;
	return sub;
}

void freeProgram(Program *program) {
	for (int i = 0; i < program->body->l; i++) {
		Typed *t = program->body->a[i];
		switch (t->type) {
			case PROGRAM:
				break;
			case IMPORT_DECLARATION:
				break;
			case IMPORT_SPECIFIER:
				break;
			case LITERAL:
				free(((Literal *)t)->value);
				free(t);
				break;
			default:
				break;
		}
	}
}

Program *initProgram(int s) {
	Program *prg = malloc(sizeof(Program));
	prg->type	 = PROGRAM;
	prg->body	 = newArray(s);
	return prg;
}

ImportSpecifier *initImportSpecifier(Identifier *name, Identifier *local) {
	ImportSpecifier *spec = malloc(sizeof(ImportSpecifier));
	spec->type			  = IMPORT_SPECIFIER;
	spec->name			  = name;
	spec->local			  = local;
	return spec;
}

ImportDeclaration *initImportDeclaration(StringLiteral *src, Array *speca) {
	ImportDeclaration *decl = malloc(sizeof(ImportSpecifier));
	decl->type				= IMPORT_DECLARATION;
	decl->source			= src;
	decl->specifiers		= speca;
	return decl;
}

ClassDeclaration *initClassDeclaration(Identifier *n, Array *b) {
	ClassDeclaration *decl = malloc(sizeof(ClassDeclaration));
	decl->type			   = CLASS_DECLARATION;
	decl->name			   = n;
	decl->body			   = b;
	return decl;
}

AssignmentExpression *initAssignmentExpression(Typed *l, Typed *r) {
	AssignmentExpression *asgn = malloc(sizeof(AssignmentExpression));
	asgn->type				   = ASSIGNMENT_EXPRESSION;
	asgn->left				   = l;
	asgn->right				   = r;
	return asgn;
}

Literal *initLiteral(char *v) {
	Literal *ltr = malloc(sizeof(Literal));
	ltr->type	 = LITERAL;
	ltr->value	 = v;
	return ltr;
}

StringLiteral *initStringLiteral(char *v) {
	StringLiteral *ltr = malloc(sizeof(StringLiteral));
	ltr->type		   = STRING_LITERAL;
	ltr->value		   = v;
	return ltr;
}

Identifier *initIdentifier(char *n) {
	Identifier *id = malloc(sizeof(Identifier));
	id->type	   = IDENTIFIER;
	id->name	   = n;
	return id;
}

TypedIdentifier *initTypedIdentifier(char *n, char *k) {
	TypedIdentifier *id = malloc(sizeof(TypedIdentifier));
	id->type			= TYPED_IDENTIFIER;
	id->name			= n;
	id->kind			= k;
	return id;
}

FunctionDeclaration *initFunctionDeclaration(
	boolean a, Identifier *n, Array *p, Array *b) {
	FunctionDeclaration *decl = malloc(sizeof(FunctionDeclaration));
	decl->type				  = FUNCTION_DECLARATION;
	decl->async				  = a;
	decl->name				  = n;
	decl->body				  = b;
	decl->params			  = p;
	return decl;
}

CallExpression *initCallExpression(Identifier *c, Array *p) {
	CallExpression *expr = malloc(sizeof(CallExpression));
	expr->type			 = CALL_EXPRESSION;
	expr->callee		 = c;
	expr->params		 = p;
	return expr;
}

BinaryExpression *initBinaryExpression(BinaryOperator o, Typed *l, Typed *r) {
	BinaryExpression *expr = malloc(sizeof(BinaryExpression));
	expr->type			   = BINARY_EXPRESSION;
	expr->left			   = l;
	expr->right			   = r;
	expr->oper			   = o;
	return expr;
}

ExpressionStatement *initExpressionStatement(Typed *e) {
	ExpressionStatement *stmt = malloc(sizeof(ExpressionStatement));
	stmt->type				  = EXPRESSION_STATEMENT;
	stmt->expr				  = e;
	return stmt;
}

ReturnStatement *initReturnStatement(Typed *e) {
	ReturnStatement *stmt = malloc(sizeof(ReturnStatement));
	stmt->type            = RETURN_STATEMENT;
	stmt->expr            = e;
	return stmt;
}