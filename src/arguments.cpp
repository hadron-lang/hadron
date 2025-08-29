#include "arguments.hpp"
#include "logger.hpp"

#include <cstring>

void ArgumentParser::add(
  const char *long_name, const char short_name, bool has_value) {
  args.push_back({long_name, short_name, nullptr, false, has_value});
}

Argument &ArgumentParser::find(const char *name) {
  for (auto &arg : args) {
    if (arg.long_name && strcmp(arg.long_name, name) == 0) {
      return arg;
    }
  }
  return def;
}

Argument &ArgumentParser::find_short(const char name) {
  for (auto &arg : args) {
    if (arg.short_name == name) {
      return arg;
    }
  }
  return def;
}

void ArgumentParser::parse(const int argc, char *argv[]) {
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      // scan positional argument
      positional.push_back(argv[i]);
    } else if (argv[i][1] != '-') {
      // scan short argument
      const char short_name = argv[i][1];
      Argument  &arg        = find_short(short_name);
      if (arg.short_name == '\0') {
        Logger::warn("Unknown argument");
        continue;
      }
      arg.is_set = true;
      if (arg.has_value) {
        if (argv[i][2] == '=') {
          arg.value = argv[i] + 3;
        } else if (argv[i][2] == '\0') {
          if (i + 1 >= argc) {
            Logger::fatal("Missing value for argument");
            return;
          }
          arg.value = argv[++i];
        } else {
          arg.value = argv[i] + 2;
        }
      } else if (argv[i][2] != '\0') {
        Logger::warn("Argument does not accept a value");
      }
    } else {
      // scan long argument
      const char *long_name = argv[i] + 2;
      char       *value     = strchr(argv[i] + 2, '=');
      if (value) {
        *value++ = '\0';
      }
      Argument &arg = find(long_name);
      if (arg.long_name == nullptr) {
        Logger::warn("Unknown argument");
        continue;
      }
      arg.is_set = true;
      if (arg.has_value) {
        if (value) {
          arg.value = value;
        } else {
          if (i + 1 >= argc) {
            Logger::fatal("Missing value for argument");
            return;
          }
          arg.value = argv[++i];
        }
      } else if (value) {
        Logger::warn("Argument does not accept a value");
      }
    }
  }
}

char *ArgumentParser::get(const char *name) { return find(name).value; }

bool ArgumentParser::is_set(const char *name) { return find(name).is_set; }
