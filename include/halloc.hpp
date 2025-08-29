#ifndef HADRON_HALLOC_H
#define HADRON_HALLOC_H

// #include "logger.h"

#define MAX_ALLOCATIONS 0x800UL

static uint8_t alloc_buffer[MAX_ALLOCATIONS]{};
static size_t  allocations = 0;

inline void *halloc(const size_t size) {
  // Avoiding dynamic allocations
  void *ptr = alloc_buffer + allocations;
  allocations += size;
  if (allocations > MAX_ALLOCATIONS) {
    Logger::fatal("Too many allocations");
  }
  // char message[96];
  // snprintf(message, 96, "Allocating %3lu bytes. Remaining %4lu", size,
  //   MAX_ALLOCATIONS - allocations);
  // Logger::warn(message);
  return ptr;
}

#endif // HADRON_HALLOC_H
