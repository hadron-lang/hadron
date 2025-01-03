#ifndef HADRON_INPUT_H
#define HADRON_INPUT_H 1

#include "file.h"

enum class InputType : uint8_t {
  FILE,
  STRING,
};

static File def;

class Input {
  const InputType type;
  size_t          index{0};
  size_t          length{0};
  char            current_char{'\0'};
  bool            end{false};
  const char     *source{nullptr};
  File           &file{def};

  public:
  explicit Input(File &file) : type(InputType::FILE), file(file) {}
  explicit Input(const char *source)
    : type(InputType::STRING), length(h_strlen(source)), source(source) {}
  Input &operator=(const Input &input) {
    if (this == &input)
      return *this;
    index        = input.index;
    length       = input.length;
    current_char = input.current_char;
    end          = input.end;
    source       = input.source;
    file         = input.file;
    return *this;
  }
  ~Input();

  char               next();
  [[nodiscard]] char peek() const;
  [[nodiscard]] char current() const;
  void               read_chunk(char *dest, size_t start, size_t length) const;
};

#endif // HADRON_INPUT_H
