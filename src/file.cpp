#include "file.h"
#include "logger.h"

#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static int open_file(const char *file_name, const int flags) {
  struct stat lstat_info{};
  struct stat fstat_info{};
  int         fd;

  if (!(flags & O_CREAT)) {
    if (lstat(file_name, &lstat_info) == -1) {
      Logger::fatal("File does not exist");
      return -1;
    }
    if (!(S_ISREG(lstat_info.st_mode))) {
      Logger::fatal("File is not a regular file");
      return -1;
    }
  }

  if ((fd = open(file_name, flags | O_NOFOLLOW | O_CLOEXEC, 0644)) == -1) {
    Logger::fatal("File could not be opened");
    return -1;
  }

  if (!(flags & O_CREAT)) {
    if (fstat(fd, &fstat_info) == -1) {
      Logger::fatal("Error opening file");
      close(fd);
      return -1;
    }

    if (lstat_info.st_ino != fstat_info.st_ino ||
        lstat_info.st_dev != fstat_info.st_dev) {
      Logger::fatal("Incompatible file type");
      close(fd);
      return -1;
    }
  }

  return fd;
}

File::File(const char *file_name, const FileMode mode) {
  this->mode      = mode;
  this->file_name = file_name;

  const int fd = open_file(file_name,
    mode == FILE_MODE_READ ? O_RDONLY : O_WRONLY | O_CREAT | O_TRUNC);

  if ((fp = fdopen(fd, mode == FILE_MODE_READ ? "rb" : "wb")) == nullptr) {
    Logger::fatal("File could not be opened");
    return;
  }

  if (mode == FILE_MODE_READ) {
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
  }
}

void File::close() {
  if (mode == FILE_MODE_WRITE && buffer_size) {
    if (write_flush() != FILE_STATUS_OK) {
      Logger::fatal("Flush error");
    }
  }
  if (fp)
    fclose(fp);
  fp = nullptr;
}

FileResult File::read() {
  if (mode != FILE_MODE_READ) {
    return FILE_MODE_INVALID;
  }
  if (eof) {
    Logger::fatal("Attempting to read after end of file");
  }

  buffer_pos             = 0;
  const size_t read_size = fread(buffer, sizeof(char), CHUNK_SIZE, fp);
  buffer_size            = read_size;
  position += read_size;
  if (read_size < CHUNK_SIZE) {
    if (feof(fp)) {
      eof = true;
      return FILE_READ_DONE;
    }
    return FILE_READ_FAILURE;
  }
  return FILE_STATUS_OK;
}

FileResult File::read_byte(char *ptr) {
  if (mode != FILE_MODE_READ) {
    return FILE_MODE_INVALID;
  }
  if (buffer_pos == buffer_size) {
    if (eof) {
      *ptr = '\0';
      return FILE_READ_DONE;
    }
    if (read() == FILE_READ_FAILURE) {
      *ptr = '\0';
      return FILE_READ_FAILURE;
    }
  }
  *ptr = static_cast<char>(buffer[buffer_pos++]);
  return FILE_STATUS_OK;
}

FileResult File::write_flush() {
  if (!fp)
    return FILE_WRITE_FAILURE;
  if (const size_t write_size = fwrite(buffer, sizeof(char), buffer_size, fp);
    write_size != buffer_size) {
    return FILE_WRITE_FAILURE;
  }
  buffer_size = 0;
  return FILE_STATUS_OK;
}

constexpr char magic[FILE_HEADER_MAGIC_SIZE] = {'\x7F', 'H', 'B', 'C'};

FileResult File::write_header() const {
  if (mode != FILE_MODE_WRITE) {
    return FILE_MODE_INVALID;
  }
  FileHeader header;
  h_memcpy(header.magic, magic, FILE_HEADER_MAGIC_SIZE);
  header.major = 0;
  header.minor = 1;
  header.flags = 0;
  header.name  = name_length();
  if (fwrite(&header, sizeof(FileHeader), 1, fp) != 1)
    return FILE_WRITE_FAILURE;
  return FILE_STATUS_OK;
}

FileResult File::read_header(FileHeader *header) {
  if (mode != FILE_MODE_READ) {
    return FILE_MODE_INVALID;
  }
  if (position != 0) { // header should be read first
    return FILE_READ_FAILURE;
  }
  fread(header, sizeof(FileHeader), 1, fp);
  if (strncmp(magic, header->magic, 4) != 0) {
    Logger::fatal("File header magic not correct");
  }
  position += sizeof(FileHeader);
  return FILE_STATUS_OK;
}

FileResult File::read_name(char *name, const size_t length) {
  if (mode != FILE_MODE_READ) {
    return FILE_MODE_INVALID;
  }
  fread(name, length, 1, fp);
  name[length] = '\0';
  position += length;
  return FILE_STATUS_OK;
}

FileResult File::lookup_byte(char *pc) const {
  if (mode != FILE_MODE_READ) {
    return FILE_MODE_INVALID;
  }
  if (buffer_pos == buffer_size) {
    const long   current   = ftell(fp);
    const size_t read_size = fread(pc, sizeof(char), 1, fp);
    fseek(fp, current, SEEK_SET);
    return read_size ? FILE_STATUS_OK : FILE_READ_FAILURE;
  }
  *pc = static_cast<char>(*(buffer + buffer_pos));
  return FILE_STATUS_OK;
}

FileResult File::current_byte(char *pc) const {
  *pc = static_cast<char>(*(buffer + buffer_pos - 1));
  return FILE_STATUS_OK;
}

FileResult File::read_chunk(
  char *buffer, const size_t start, const size_t length) const {
  const long current = ftell(fp);
  fseek(fp, static_cast<long>(start), SEEK_SET);
  const size_t read_size = fread(buffer, sizeof(char), length, fp);
  buffer[read_size]      = '\0';
  fseek(fp, current, SEEK_SET);
  return FILE_STATUS_OK;
}

void File::get_dir(char *path) const {
  int k = 0; // save last '/'
  for (int i = 0; file_name[i]; i++) {
    path[i] = file_name[i];
    if (file_name[i] == '/')
      k = i;
  }
  path[k] = '\0';
}

uint8_t File::name_length() const {
  int j = 0;
  int k = 0;
  for (int i = 0; file_name[i]; i++) {
    if (file_name[i] == '/')
      j = i + 1;
    if (file_name[i] == '.')
      k = i;
  }
  return k - j;
}

void File::get_name(char *name) const {
  int j = 0; // save last '/'
  int k = 0; // save last '.'
  for (int i = 0; file_name[i]; i++) {
    if (file_name[i] == '/')
      j = i + 1;
    if (file_name[i] == '.')
      k = i;
  }
  for (int i = j; i < k; i++) {
    name[i - j] = file_name[i];
  }
  name[k - j] = 0;
}

void File::get_ext(char *extension) const {
  int j = 0; // save last '.'
  int i = 0;
  for (i = 0; file_name[i]; i++) {
    if (file_name[i] == '.')
      j = i + 1;
  }
  for (i = j; file_name[i]; i++) {
    extension[i - j] = file_name[i];
  }
  extension[i - j] = 0;
}
