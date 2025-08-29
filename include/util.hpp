#ifndef HADRON_MEMORY_H
#define HADRON_MEMORY_H 1

#include <cstddef>

void  *h_memcpy(void *dst, const void *src, size_t n);
int    h_memcpy_s(void *dst, size_t dst_sz, const void *src, size_t n);
size_t h_strlen(const char *s);
size_t h_strnlen(const char *s, size_t max_len);

#endif // HADRON_MEMORY_H
