#include "input.h"
#include "util.h"

Input::~Input() = default;

char Input::next() {
  if (type == InputType::FILE) {
    char c;
    if (file.read_byte(&c) == FILE_READ_FAILURE) {
      printf("File read failure\n");
      end          = true;
      current_char = '\0';
    }
    return current_char = c;
  }
  if (index >= length) {
    return '\0';
  }
  return current_char = source[index++];
}

char Input::peek() const {
  if (type == InputType::FILE) {
    char c;
    if (file.lookup_byte(&c) == FILE_STATUS_OK)
      return c;
    return '\0';
  }
  if (index >= length)
    return '\0';
  return source[index];
}

char Input::current() const { return current_char; }

void Input::read_chunk(
  char *dest, const size_t start, const size_t length) const {
  if (type == InputType::FILE) {
    file.read_chunk(dest, start, length);
    return;
  }
  h_memcpy(dest, source + start, length);
  dest[length] = '\0';
}
