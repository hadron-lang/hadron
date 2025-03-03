#ifndef HADRON_READER_H
#define HADRON_READER_H 1

#define CHUNK_SIZE 1024

#include "util.h"
#include "logger.h"

#include <cstdint>
#include <cstdio>
#include <type_traits>

typedef enum __attribute__((__packed__)) FileMode {
  FILE_MODE_NONE,
  FILE_MODE_READ,
  FILE_MODE_WRITE,
} FileMode;

typedef enum __attribute__((__packed__)) FileResult {
  FILE_STATUS_OK,
  FILE_MODE_INVALID,
  FILE_READ_DONE,
  FILE_READ_FAILURE,
  FILE_WRITE_FAILURE,
} FileResult;

#define FILE_HEADER_MAGIC_SIZE 4
#define MAX_WRITE_LENGTH       0x1000

typedef struct FileHeader {
  char    magic[FILE_HEADER_MAGIC_SIZE];
  uint8_t major;
  uint8_t minor;
  uint8_t flags;
  uint8_t name;
} FileHeader;

class File {
  uint8_t buffer[CHUNK_SIZE]{};
public:
  FILE       *fp{nullptr};
  const char *file_name{nullptr};
private:
  size_t      file_size{0};
  size_t      buffer_size{0};
  size_t      buffer_pos{0};
  size_t      position{0};

  bool       eof{false};
  FileMode   mode{};
  FileResult write_flush();
  void       close();

  [[nodiscard]] uint8_t name_length() const;

  public:
  File() = default;
  File(const char *file_name, FileMode mode);
  ~File() { close(); }

  void get_dir(char *path) const;
  void get_name(char *name) const;
  void get_ext(char *extension) const;

  FileResult read();
  FileResult read_byte(char *pc);
  FileResult lookup_byte(char *pc) const;
  FileResult current_byte(char *pc) const;
  FileResult read_chunk(char *buffer, size_t start, size_t length) const;

  [[nodiscard]] FileResult write_header() const;
  [[nodiscard]] FileResult read_header(FileHeader *header);
  [[nodiscard]] FileResult read_name(char *name, size_t length);

  template <typename T> FileResult write(T value) {
    if (mode != FILE_MODE_WRITE) {
      return FILE_MODE_INVALID;
    }
    if constexpr (std::is_same_v<T, char *>) {
      const size_t length = h_strnlen(value, MAX_WRITE_LENGTH);
      if (buffer_size + length >= CHUNK_SIZE) {
        if (const FileResult res = write_flush(); res != FILE_STATUS_OK) {
          return res;
        }
      }
      if (const size_t write_size = fwrite(value, 1, length, fp);
        write_size != length) {
        return FILE_WRITE_FAILURE;
      }
      return FILE_STATUS_OK;
    } else {
      if (buffer_size + sizeof(value) == CHUNK_SIZE) {
        if (const FileResult res = write_flush(); res != FILE_STATUS_OK) {
          return res;
        }
      }
      buffer[buffer_size] = static_cast<uint8_t>(value);
      buffer_size += sizeof(value);
      return FILE_STATUS_OK;
    }
  }

  template <typename T> File &operator<<(T value) {
    if (const FileResult res = write(value); res != FILE_STATUS_OK) {
      Logger::fatal("File write failed");
    }
    return *this;
  }
};

#endif // HADRON_READER_H
