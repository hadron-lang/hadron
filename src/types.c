#include "types.h"

TArray *initTArray(int length) {
	TArray *array = malloc(sizeof(TArray));
	array->array = malloc(sizeof(Token *) * length);
	array->length = length;
	return array;
}

void pushToken(TArray *ptr, Token *token) {
	ptr->array = realloc(ptr->array, sizeof(Token *) * (ptr->length+1));
	ptr->array[ptr->length] = token;
	ptr->length++;
}

void freeTArray(TArray *ptr) {
	free(ptr->array);
}
