#include "main.h"

int main(int argc, string *argv) {
	Args args = { 0, NULL, 0 };
	bool mode_configured = false;
	for (small i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--compile") == 0) {
			if (mode_configured && !args.mode)
				return CLIError(3, "\x1b[96m--interpret", " and \x1b[96m--compile", " are mutually exclusive");
			args.mode = 1;
			mode_configured = true;
		} else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interpret") == 0) {
			if (args.lang) return CLIError(1, "language is not configurable in interpreter");
			if (mode_configured && args.mode)
				return CLIError(3, "\x1b[96m--interpret", " and \x1b[96m--compile", " are mutually exclusive");
			args.mode = 0;
			mode_configured = true;
		} else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lang") == 0) {
			if (isLang(argv[i+1]));
			args.lang = parseLang(argv[++i]);
		} else if (isFile(argv[i])) args.file = argv[i];
	}
	if (!args.mode && args.lang) return CLIError(1, "language is not available in interpreter");
	if (/*args.mode &&*/ !args.file) return CLIError(1, "you should provide at least one file to compile");
	if (args.mode && !args.lang) return CLIError(1, "you should choose one language to compile to");

	FILE *fp = fopen(args.file, "r");
	if (fp == NULL) {
		string err1 = "file\x1b[97m ";
		string err = malloc(strlen(err1) + strlen(args.file));
		strcat(err, err1);
		strcat(err, args.file);
		CLIError(2, err, " does not exist");
		free(err);
		return -1;
	}

	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, 0);

	string contents = malloc(fsize);
	fread(contents, fsize, true, fp);

	fclose(fp);

	TArray *t = tokenize(contents);
	printf("final length %i\n", t->length);
	printf("final size %i\n", t->size);
	printTokens(contents, t);

	free(contents);
	freeTArray(t);
}

int CLIError(int argcount, ...) {
	va_list v;
	va_start(v, argcount);
	printf("\x1b[7;1;91m CLIError \x1b[0;91m ");
	for (small i = 1; i <= argcount; i++) {
		printf("%s\x1b[0;91m", va_arg(v, string));
	}
	va_end(v);
	printf("\x1b[0m\n");
	return -1;
}
bool isLang(string arg) {
	for (int i = 0; i < strlen(arg); i++) {
		if (
			!((arg[i] >= 'a' && arg[i] <= 'z')
			|| (arg[i] >= 'A' && arg[i] <= 'Z')
			|| arg[i] == '+')
		) return false;
	}
	return true;
}
bool isFile(string arg) {
	for (int i = 0; i < strlen(arg); i++) {
		if (!(
			(arg[i] >= 'a' && arg[i] <= 'z')
			|| (arg[i] >= 'A' && arg[i] <= 'Z')
			|| (arg[i] >= '0' && arg[i] <= '9')
			|| arg[i] == '+' || arg[i] == '.'
			|| arg[i] == '$' || arg[i] == '~'
			|| arg[i] == '-' || arg[i] == '\\'
			|| arg[i] == '/'
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
