#include "types.h"

char *getTokenContent(char *src, Token *t) {
  if (!t) return NULL;
  boolean str = t->type == STR;
  int     l   = t->pos.absEnd - t->pos.absStart - (str ? 2 : 0);
  char   *sub = (char *)malloc(l + 1);
  memcpy(sub, src + t->pos.absStart + (str ? 1 : 0), l);
  sub[l] = 0;
  return sub;
}

void freeProgram(Program *program) {
  for (int i = 0; i < program->body->length; i++) {
    Typed *t = program->body->array[i];
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
  prg->type    = PROGRAM;
  prg->body    = newArray(s);
  return prg;
}

ImportSpecifier *initImportSpecifier(Identifier *name, Identifier *local) {
  ImportSpecifier *spec = malloc(sizeof(ImportSpecifier));
  spec->type            = IMPORT_SPECIFIER;
  spec->name            = name;
  spec->local           = local;
  return spec;
}

ImportDeclaration *initImportDeclaration(StringLiteral *src, Array *speca) {
  ImportDeclaration *decl = malloc(sizeof(ImportSpecifier));
  decl->type              = IMPORT_DECLARATION;
  decl->source            = src;
  decl->specifiers        = speca;
  return decl;
}

ClassDeclaration *initClassDeclaration(Identifier *n, Array *b) {
  ClassDeclaration *decl = malloc(sizeof(ClassDeclaration));
  decl->type             = CLASS_DECLARATION;
  decl->name             = n;
  decl->body             = b;
  return decl;
}

AssignmentExpression *initAssignmentExpression(
  Typed *l, Typed *r, AssignmentOperator oper) {
  AssignmentExpression *asgn = malloc(sizeof(AssignmentExpression));
  asgn->type                 = ASSIGNMENT_EXPRESSION;
  asgn->left                 = l;
  asgn->right                = r;
  asgn->oper                 = oper;
  return asgn;
}

Literal *initLiteral(char *v) {
  Literal *ltr = malloc(sizeof(Literal));
  ltr->type    = LITERAL;
  ltr->value   = v;
  return ltr;
}

StringLiteral *initStringLiteral(char *v) {
  StringLiteral *ltr = malloc(sizeof(StringLiteral));
  ltr->type          = STRING_LITERAL;
  ltr->value         = v;
  return ltr;
}

Identifier *initIdentifier(char *n) {
  Identifier *id = malloc(sizeof(Identifier));
  id->type       = IDENTIFIER;
  id->name       = n;
  return id;
}

TypedIdentifier *initTypedIdentifier(char *n, char *k) {
  TypedIdentifier *id = malloc(sizeof(TypedIdentifier));
  id->type            = TYPED_IDENTIFIER;
  id->name            = n;
  id->kind            = k;
  return id;
}

FunctionDeclaration *initFunctionDeclaration(
  boolean a, Identifier *n, Array *p, Array *b) {
  FunctionDeclaration *decl = malloc(sizeof(FunctionDeclaration));
  decl->type                = FUNCTION_DECLARATION;
  decl->async               = a;
  decl->name                = n;
  decl->body                = b;
  decl->params              = p;
  return decl;
}

CallExpression *initCallExpression(Identifier *c, Array *p) {
  CallExpression *expr = malloc(sizeof(CallExpression));
  expr->type           = CALL_EXPRESSION;
  expr->callee         = c;
  expr->params         = p;
  return expr;
}

BinaryExpression *initBinaryExpression(BinaryOperator o, Typed *l, Typed *r) {
  BinaryExpression *expr = malloc(sizeof(BinaryExpression));
  expr->type             = BINARY_EXPRESSION;
  expr->left             = l;
  expr->right            = r;
  expr->oper             = o;
  return expr;
}

UnaryExpression *initUnaryExpression(UnaryOperator o, Typed *e) {
  UnaryExpression *expr = malloc(sizeof(UnaryExpression));
  expr->type            = UNARY_EXPRESSION;
  expr->oper            = o;
  expr->expr            = e;
  return expr;
}

ExpressionStatement *initExpressionStatement(Typed *e) {
  ExpressionStatement *stmt = malloc(sizeof(ExpressionStatement));
  stmt->type                = EXPRESSION_STATEMENT;
  stmt->expr                = e;
  return stmt;
}

ReturnStatement *initReturnStatement(Typed *e) {
  ReturnStatement *stmt = malloc(sizeof(ReturnStatement));
  stmt->type            = RETURN_STATEMENT;
  stmt->expr            = e;
  return stmt;
}

SwitchStatement *initSwitchStatement(Typed *e) {
  SwitchStatement *stmt = malloc(sizeof(SwitchStatement));
  stmt->type            = SWITCH_STATEMENT;
  stmt->expr            = e;
  return stmt;
}

CaseStatement *initCaseStatement(Typed *t, Array *b) {
  CaseStatement *stmt = malloc(sizeof(CaseStatement));
  stmt->type          = CASE_STATEMENT;
  stmt->test          = t;
  stmt->body          = b;
  return stmt;
}

DefaultStatement *initDefaultStatement(Array *b) {
  DefaultStatement *stmt = malloc(sizeof(DefaultStatement));
  stmt->type             = DEFAULT_STATEMENT;
  stmt->body             = b;
  return stmt;
}

ForStatement *initForStatement(Typed *i, Typed *t, Typed *u, Array *b) {
  ForStatement *stmt = malloc(sizeof(ForStatement));
  stmt->type         = FOR_STATEMENT;
  stmt->init         = i;
  stmt->test         = t;
  stmt->update       = u;
  stmt->body         = b;
  return stmt;
}

WhileStatement *initWhileStatement(Typed *t, Array *b) {
  WhileStatement *stmt = malloc(sizeof(WhileStatement));
  stmt->type           = WHILE_STATEMENT;
  stmt->test           = t;
  stmt->body           = b;
  return stmt;
}
