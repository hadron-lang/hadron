#include "util.hpp"

void *h_memcpy(void *dst, const void *src, const size_t n) {
  for (size_t i = 0; i < n; i++) {
    static_cast<char *>(dst)[i] = static_cast<const char *>(src)[i];
  }
  return dst;
}

int h_memcpy_s(
  void *dst, const size_t dst_sz, const void *src, const size_t sz) {
  if (!dst || !src || sz > dst_sz) {
    return -1;
  }
  for (size_t i = 0; i < sz; i++) {
    static_cast<char *>(dst)[i] = static_cast<const char *>(src)[i];
  }
  return 0;
}

size_t h_strlen(const char *s) {
  if (!s || !*s) {
    return 0;
  }
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

size_t h_strnlen(const char *s, const size_t max_len) {
  if (!s || !max_len) {
    return 0;
  }
  size_t len = 0;
  while (len < max_len && s[len]) {
    len++;
  }
  return len;
}
