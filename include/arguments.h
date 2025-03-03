#ifndef HADRON_ARGUMENTS_H
#define HADRON_ARGUMENTS_H 1

#include <vector>

struct Argument {
  const char *long_name;
  char        short_name;
  char       *value;
  bool        is_set;
  bool        has_value;
};

class ArgumentParser {
  Argument &find(const char *name);
  Argument &find_short(char name);

  Argument def{};

  public:
  std::vector<Argument> args{};
  std::vector<char *>   positional{};

  void  add(const char *long_name, char short_name, bool has_value = true);
  void  parse(int argc, char *argv[]);
  char *get(const char *name);
  bool  is_set(const char *name);
};

#endif // HADRON_ARGUMENTS_H
