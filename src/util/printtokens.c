#include "printtokens.h"

void printTokens(string code, TArray *tokens) {
	for (int i = 0; i < tokens->length; i++) {
		int len = tokens->array[i]->end - tokens->array[i]->start;
		string substr = malloc(len+1);
		memcpy(substr, &code[tokens->array[i]->start], len);
		substr[len] = '\0';
		printf("type: %i \x1b[7m%s\x1b[0m\n", tokens->array[i]->type, substr);
		free(substr);
	}
}
