#include "main.h"

static int check(Array *, bool);
static bool isLang(string);
static bool isFile(string);
static Lang parseLang(string);

int main(int argc, string *argv) {
	Args args = { 0, 0, false, NULL };
	Array *errors = newArray(1);
	for (small i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0) {
			if (args.set && !args.mode)
				pushArray(errors, clierror(3, "\x1b[96m--interpret", " and \x1b[96m--compile", " are mutually exclusive"));
			args.mode = 1;
			args.set = true;
		} else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interpret") == 0) {
			if (args.lang) pushArray(errors, clierror(1, "language is not configurable in interpreter"));
			if (args.set && args.mode)
				pushArray(errors, clierror(3, "\x1b[96m--interpret", " and \x1b[96m--compile", " are mutually exclusive"));
			args.mode = 0;
			args.set = true;
		} else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lang") == 0) {
			if (isLang(argv[i+1])) args.lang = parseLang(argv[++i]);
		} else if (isFile(argv[i])) args.file = argv[i];
	}
	if (!args.mode && args.lang) pushArray(errors, clierror(1, "language is not available in interpreter"));
	if (/*args.mode &&*/ !args.file) pushArray(errors, clierror(1, "you should provide at least one file to compile"));
	if (args.mode && !args.lang) pushArray(errors, clierror(1, "you should choose one language to compile to"));
	if (check(errors, false)) return -1;
	FILE *fp = fopen(args.file, "rb");
	if (fp == NULL) {
		string e0 = "file \x1b[97m";
		string e = malloc(strlen(e0) + strlen(args.file) + 1);
		strcpy(e, e0);
		strcat(e, args.file);
		pushArray(errors, clierror(2, e, " does not exist"));
		free(e);
	}
	if (check(errors, true)) return -1;

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	rewind(fp);

	string contents = malloc(fsize);
	fread(contents, fsize, true, fp);
	contents[fsize-1] = 0;

	fclose(fp);

	Result *t = tokenize(contents, args.file);
	if (check(t->errors, true)) return -1;

	// printTokens(contents, t->data);

	Result *p = parse(contents, t->data, args.file);
	if (check(p->errors, true)) return -1;

	printAST(p->data, 2);

	free(contents);
	freeArray(t->data);
	free(t);
	freeArray(((Program *)p->data)->body);
	free(p);
}

int check(Array *errors, bool drop) {
	if (errors->l) {
		printErrors(errors);
		if (drop) freeArray(errors);
		return 1;
	}; if (drop) freeArray(errors);
	return 0;
}

bool isLang(string arg) {
	for (size_t i = 0; i < strlen(arg); i++) {
		if (
			!((arg[i] >= 'a' && arg[i] <= 'z')
			|| (arg[i] >= 'A' && arg[i] <= 'Z')
			|| arg[i] == '+')
		) return false;
	}
	return true;
}
bool isFile(string arg) {
	for (size_t i = 0; i < strlen(arg); i++) {
		if (!(
			(arg[i] >= 'a' && arg[i] <= 'z')
			|| (arg[i] >= 'A' && arg[i] <= 'Z')
			|| (arg[i] >= '0' && arg[i] <= '9')
			|| arg[i] == '+' || arg[i] == '.'
			|| arg[i] == '$' || arg[i] == '~'
			|| arg[i] == '-' || arg[i] == '\\'
			|| arg[i] == '/' || arg[i] == '_'
		)) return false;
	}
	return true;
}

Lang parseLang(string arg) {
	if (strcasecmp(arg, "js") == 0 || strcasecmp(arg, "javascript") == 0) {
		return L_JS;
	} else if (strcasecmp(arg, "asm") == 0 || strcasecmp(arg, "assembly") == 0) {
		return L_ASM;
	} else if (strcasecmp(arg, "c") == 0) {
		return L_C;
	}
	return L_UNKN;
}
