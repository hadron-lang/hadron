#include "types.h"

void initTArray(TArray *ptr) {
	ptr->array = malloc(0);
	ptr->length = 0;
}

void pushToken(TArray *ptr, Token token) {
	ptr->array = realloc(ptr->array, sizeof(Token) * (ptr->length+1));
	ptr->array[ptr->length] = token;
	ptr->length++;
}

void freeTArray(TArray *ptr) {
	free(ptr->array);
}
