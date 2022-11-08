#include "types.h"

TArray *initTArray(int size) {
	TArray *array = malloc(sizeof(TArray));
	array->array = malloc(sizeof(Token *) * size);
	array->length = 0;
	array->size = size;
	printf("init at %i\n", size);
	return array;
}

void pushToken(TArray *ptr, Token *token) {
	if (ptr->size == ptr->length) {
		ptr->array = realloc(ptr->array, sizeof(Token *) * (ptr->size*=1.5));
		printf("resize to %i\n", ptr->size);
	}
	ptr->array[ptr->length++] = token;
}

void freeTArray(TArray *ptr) {
	for (int i = 0; i < ptr->length; i++) free(ptr->array[i]);
	free(ptr->array);
}
