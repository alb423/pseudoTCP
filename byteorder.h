#ifndef __BYTEORDER_H__
#define __BYTEORDER_H__

#include "mi_types.h"

// Reading and writing of little and big-endian numbers from memory
// TODO: Optimized versions, with direct read/writes of
// integers in host-endian format, when the platform supports it.

static inline void Set8(void* memory, size_t offset, U8 v) {
  ((U8*)(memory))[offset] = v;
}

static inline U8 Get8(const void* memory, size_t offset) {
  return ((const U8*)(memory))[offset];
}

static inline void SetBE16(void* memory, U16 v) {
  Set8(memory, 0, (U8)(v >> 8));
  Set8(memory, 1, (U8)(v >> 0));
}

static inline void SetBE32(void* memory, U32 v) {
  Set8(memory, 0, (U8)(v >> 24));
  Set8(memory, 1, (U8)(v >> 16));
  Set8(memory, 2, (U8)(v >> 8));
  Set8(memory, 3, (U8)(v >> 0));
}

static inline U16 GetBE16(const void* memory) {
  return (U16)((Get8(memory, 0) << 8) |
                             (Get8(memory, 1) << 0));
}

static inline U32 GetBE32(const void* memory) {
  return ((U32)(Get8(memory, 0)) << 24) |
      ((U32)(Get8(memory, 1)) << 16) |
      ((U32)(Get8(memory, 2)) << 8) |
      ((U32)(Get8(memory, 3)) << 0);
}

static inline void SetLE16(void* memory, U16 v) {
  Set8(memory, 0, (U8)(v >> 0));
  Set8(memory, 1, (U8)(v >> 8));
}

static inline void SetLE32(void* memory, U32 v) {
  Set8(memory, 0, (U8)(v >> 0));
  Set8(memory, 1, (U8)(v >> 8));
  Set8(memory, 2, (U8)(v >> 16));
  Set8(memory, 3, (U8)(v >> 24));
}

static inline U16 GetLE16(const void* memory) {
  return (U16)((Get8(memory, 0) << 0) |
                             (Get8(memory, 1) << 8));
}

static inline U32 GetLE32(const void* memory) {
  return ((U32)(Get8(memory, 0)) << 0) |
      ((U32)(Get8(memory, 1)) << 8) |
      ((U32)(Get8(memory, 2)) << 16) |
      ((U32)(Get8(memory, 3)) << 24);
}


// Check if the current host is big endian.
static inline bool IsHostBigEndian() {
  static const int number = 1;
  return 0 == *(const char*)(&number);
}

static inline U16 HostToNetwork16(U16 n) {
  U16 result;
  SetBE16(&result, n);
  return result;
}

static inline U32 HostToNetwork32(U32 n) {
  U32 result;
  SetBE32(&result, n);
  return result;
}

static inline U16 NetworkToHost16(U16 n) {
  return GetBE16(&n);
}

static inline U32 NetworkToHost32(U32 n) {
  return GetBE32(&n);
}



#endif  // __BYTEORDER_H__
