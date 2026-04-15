/* arena.h - v1.0 - public domain arena allocator
   no warranty implied; use at your own risk

   This file provides a simple arena allocator that allocates from a
   contiguous block of memory. All allocations are freed together when
   the arena is reset or destroyed.

   Usage:
      #define ARENA_IMPLEMENTATION
      #include "arena.h"

   The implementation can be included in multiple source files as long
   as ARENA_IMPLEMENTATION is defined in exactly one compilation unit.

   Example:
      // In exactly one .c file:
      #define ARENA_IMPLEMENTATION
      #include "arena.h"

      // In other .c/.h files:
      #include "arena.h"

      // Basic usage
      Arena arena;
      if (!arena_init(&arena, 1 * MiB)) {  // 1MB arena
          // handle out of memory
      }

      // Allocate a single object
      int *p = arena_new(&arena, int);
      if (p) *p = 42;

      // Allocate an array
      float *arr = arena_new_array(&arena, float, 100);
      if (arr) {
          for (int i = 0; i < 100; i++) arr[i] = (float)i;
      }

      // Allocate with custom alignment
      void *aligned = arena_alloc(&arena, 256, 64);  // 64-byte aligned

      // Reset for reuse (keeps memory, just resets used counter)
      arena_reset(&arena);

      // Allocations after reset reuse the same memory
      char *buf = arena_new_array(&arena, char, 512);

      // Clean up when done
      arena_destroy(&arena);

   Options:
      Define ARENA_ASSERT before including to use a custom assert.
      Define ARENA_MALLOC/ARENA_FREE to use custom allocators.
      Define ARENA_STATIC to make functions static.

   Version History:
      1.0  (2025-04-15)  initial version

   Credits:
      Written by Nate Helmig
      Based on stb header design by Sean Barrett
*/

#ifndef ARENA_H
#define ARENA_H

#include <cstdint>
#include <stdbool.h>
#include <stdckdint.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Configuration - define these before including or in your build */
#ifndef ARENA_ASSERT
#include <assert.h>
#define ARENA_ASSERT(x) assert(x)
#endif

#ifndef ARENA_MALLOC
#include <stdlib.h>
#define ARENA_MALLOC(sz) malloc(sz)
#endif

#ifndef ARENA_FREE
#include <stdlib.h>
#define ARENA_FREE(p) free(p)
#endif

#ifndef ARENA_MEMSET
#include <string.h>
#define ARENA_MEMSET(p, v, sz) memset(p, v, sz)
#endif

/* By default, functions are extern for use across translation units.
   Define ARENA_STATIC before including to make functions static (private).
   In the implementation file (where ARENA_IMPLEMENTATION is defined),
   functions must not be static to be visible to other translation units. */
#ifndef ARENA_STATIC
#define ARENA_DEF
#else
#define ARENA_DEF static
#endif

/* Arena handle - treat as opaque */
typedef struct {
  unsigned char *base;
  size_t capacity;
  size_t used;
} Arena;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

/* API */

/* Initialize arena with given capacity. Returns true on success. */
ARENA_DEF bool arena_init(Arena *a, size_t capacity);

/* Free all memory and reset arena to zeroed state. */
ARENA_DEF void arena_destroy(Arena *a);

/* Reset used counter to zero, keeping the memory block. */
ARENA_DEF void arena_reset(Arena *a);

/* Allocate size bytes with given alignment. Returns NULL on failure.
   align must be a power of two. */
ARENA_DEF void *arena_alloc(Arena *a, size_t size, size_t align);

/* Allocate array of count elements, each of elem_size bytes.
   Returns NULL on overflow or allocation failure. */
ARENA_DEF void *arena_alloc_array(Arena *a, size_t count, size_t elem_size,
                                  size_t align);

/* Convenience macros for allocating typed memory */
#define arena_new(a, T) ((T *)arena_alloc((a), sizeof(T), alignof(T)))
#define arena_new_array(a, T, n)                                               \
  ((T *)arena_alloc_array((a), (n), sizeof(T), alignof(T)))

#ifdef __cplusplus
}
#endif

/* -------------------------------------------------------------------- */
/*                           IMPLEMENTATION                             */
/* -------------------------------------------------------------------- */

#ifdef ARENA_IMPLEMENTATION

ARENA_DEF bool arena_init(Arena *a, size_t capacity) {
  ARENA_ASSERT(a != NULL);
  a->base = (unsigned char *)ARENA_MALLOC(capacity);
  if (a->base == NULL)
    return false;
  a->capacity = capacity;
  a->used = 0;
  return true;
}

ARENA_DEF void arena_destroy(Arena *a) {
  if (a == NULL)
    return;
  ARENA_FREE(a->base);
  ARENA_MEMSET(a, 0, sizeof(*a));
}

ARENA_DEF void arena_reset(Arena *a) {
  ARENA_ASSERT(a != NULL);
  a->used = 0;
}

ARENA_DEF bool arena__is_power_of_two(size_t x) {
  return x != 0 && (x & (x - 1)) == 0;
}

ARENA_DEF void *arena_alloc(Arena *a, size_t size, size_t align) {
  ARENA_ASSERT(a != NULL);
  ARENA_ASSERT(a->used <= a->capacity);
  ARENA_ASSERT(arena__is_power_of_two(align));

  if (a->base == NULL)
    return NULL;

  if (!arena__is_power_of_two(align))
    return NULL;

  /* Calculate aligned offset */
  uintptr_t current = (uintptr_t)(a->base + a->used);
  size_t padding = (size_t)(-(intptr_t)current & (intptr_t)(align - 1));

  size_t offset, next_used;
  if (ckd_add(&offset, a->used, padding))
    return NULL;
  if (offset > a->capacity)
    return NULL;
  if (ckd_add(&next_used, offset, size))
    return NULL;
  if (next_used > a->capacity)
    return NULL;

  void *result = a->base + offset;
  a->used = next_used;
  return result;
}

ARENA_DEF void *arena_alloc_array(Arena *a, size_t count, size_t elem_size,
                                  size_t align) {
  size_t total;
  if (ckd_mul(&total, count, elem_size))
    return NULL;
  return arena_alloc(a, total, align);
}

#endif /* ARENA_IMPLEMENTATION */

#endif /* ARENA_H */
