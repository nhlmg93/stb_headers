# stb_headers

A collection of single-header C libraries in the style of [stb](https://github.com/nothings/stb).

## Libraries

### arena.h - Arena Allocator

A simple arena allocator that allocates from a contiguous block of memory. All allocations are freed together when the arena is reset or destroyed.

**Features:**
- Single header, no dependencies beyond standard C
- Configurable via macros (custom assert, malloc/free, static linkage)
- Overflow-safe arithmetic using C23 `ckd_int.h`
- Typed allocation macros (`arena_new`, `arena_new_array`)

#### Quick Start

```c
// In exactly one .c file:
#define ARENA_IMPLEMENTATION
#include "arena.h"

// In other .c/.h files:
#include "arena.h"
```

#### Example Usage

```c
Arena arena;
if (!arena_init(&arena, 1024 * 1024)) {  // 1MB arena
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
```

#### Configuration Options

Define these *before* including `arena.h`:

| Macro | Description |
|-------|-------------|
| `ARENA_ASSERT(x)` | Custom assert macro (default: `assert(x)`) |
| `ARENA_MALLOC(sz)` | Custom allocator (default: `malloc(sz)`) |
| `ARENA_FREE(p)` | Custom deallocator (default: `free(p)`) |
| `ARENA_STATIC` | Make all functions `static` for private use |

#### API Reference

```c
// Initialize arena with given capacity. Returns true on success.
bool arena_init(Arena *a, size_t capacity);

// Free all memory and reset arena to zeroed state.
void arena_destroy(Arena *a);

// Reset used counter to zero, keeping the memory block.
void arena_reset(Arena *a);

// Allocate size bytes with given alignment. Returns NULL on failure.
// align must be a power of two.
void *arena_alloc(Arena *a, size_t size, size_t align);

// Allocate array of count elements, each of elem_size bytes.
// Returns NULL on overflow or allocation failure.
void *arena_alloc_array(Arena *a, size_t count, size_t elem_size, size_t align);

// Convenience macros for typed allocation
arena_new(a, T)           // Allocate single T
arena_new_array(a, T, n)  // Allocate array of n T's
```

#### Out of Memory Handling

`arena_init()` can fail when:
- The system cannot provide a contiguous block of the requested size
- Physical memory + swap are exhausted
- Resource limits (`ulimit`) are exceeded
- Linux overcommit is disabled (`vm.overcommit_memory = 2`)

Unlike growable allocators, arenas pre-allocate their entire capacity upfront, making OOM on init more likely for large arenas.

## License

Public domain. See individual header files for details.
