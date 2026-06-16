// SPDX-License-Identifier: LGPL-2.1-or-later
// Derived from slimcc by fuhsnn.
#include "rcc.h"
#include <stdarg.h>

//
// Arena Allocator
//
typedef struct Chunk Chunk;
struct Chunk {
    Chunk *next;
    size_t size;
    size_t used;
    char data[];
};

static Chunk *current_chunk;

void *arena_alloc(size_t size) {
    // Align to 16 bytes: satisfies struct padding and guarantees SIMD
    // loads (SSE2/NEON 16-byte reads in u8ident_check_ident_align16) never read
    // past the end of a chunk into unmapped memory.
    size = (size + 15) & ~15;

    if (!current_chunk || current_chunk->used + size > current_chunk->size) {
        size_t chunk_size = 4096 * 1024; // 4MB chunks
        if (size > chunk_size) {
            chunk_size = size;
        }
        Chunk *chunk = calloc(1, sizeof(Chunk) + chunk_size);
        chunk->next = current_chunk;
        chunk->size = chunk_size;
        current_chunk = chunk;
    }

    void *ptr = current_chunk->data + current_chunk->used;
    current_chunk->used += size;
    return ptr;
}

char *format(char *fmt, ...) {
    char buf[2048];
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    char *s = arena_alloc(len + 1);
    strcpy(s, buf);
    return s;
}

//
// String Interning (Hash Map)
//

// FNV-1a Hash function
static uint32_t hash_str(const char *s, int len) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < len; i++) {
        hash ^= (uint8_t)s[i];
        hash *= 16777619;
    }
    return hash;
}

typedef struct InternedStr InternedStr;
struct InternedStr {
    InternedStr *next;
    char *str;
    int len;
};

// Dynamic hash table: sized per input file by str_intern_resize().
// Heap-allocated so it doesn't permanently occupy cache during codegen.
static InternedStr **strings = NULL;
static uint32_t hash_size = 0;

static void ensure_str_intern_init(void) {
    if (strings)
        return;
    // Small default for early calls (keywords, -D defines) before read_file().
    hash_size = 256;
    strings = calloc(hash_size, sizeof(InternedStr *));
}

// Call after read_file() with strlen(source) before preprocess().
// Sizes the bucket array for the expected identifier count; rehashes any
// entries that were added during early init (keywords, -D flags).
void str_intern_resize(size_t src_bytes) {
    // Heuristic: ~1 unique identifier per 1000 bytes of source; load factor 0.5
    // → target = src_bytes / 500
    uint32_t new_size = 256;
    size_t target = src_bytes / 500;
    while (new_size < target && new_size < 65536)
        new_size <<= 1;

    if (!strings) {
        hash_size = new_size;
        strings = calloc(hash_size, sizeof(InternedStr *));
        return;
    }
    if (new_size <= hash_size)
        return; // already big enough (e.g. second file in multi-file build)

    // Rehash the small initial table into the larger one.
    InternedStr **ns = calloc(new_size, sizeof(InternedStr *));
    for (uint32_t i = 0; i < hash_size; i++) {
        for (InternedStr *s = strings[i]; s;) {
            InternedStr *next = s->next;
            uint32_t h = hash_str(s->str, s->len) % new_size;
            s->next = ns[h];
            ns[h] = s;
            s = next;
        }
    }
    free(strings);
    strings = ns;
    hash_size = new_size;
}

char *str_intern(const char *start, int len) {
    ensure_str_intern_init();
    uint32_t h = hash_str(start, len) % hash_size;

    for (InternedStr *s = strings[h]; s; s = s->next) {
        if (s->len == len && !memcmp(s->str, start, len))
            return s->str;
    }

    InternedStr *s = arena_alloc(sizeof(InternedStr));
    s->str = arena_alloc(len + 1);
    memmove(s->str, start, len);
    s->str[len] = '\0';
    s->len = len;

    s->next = strings[h];
    strings[h] = s;
    return s->str;
}
