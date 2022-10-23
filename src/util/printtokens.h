#include "../types.h"

void printTokens(string code, TArray* tokens) {
	for (int i = 0; i < tokens->length; i++) {
		string type;
		int len = tokens->array[i].end - tokens->array[i].start;
		string substr = malloc(len+1);
		memcpy(substr, &code[tokens->array[i].start], len);
		substr[len] = '\0';
		type = substr;
		printf("type: %i %s\n", tokens->array[i].type, type);
	}
}
