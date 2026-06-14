// SPDX-License-Identifier: LGPL-2.1-or-later
#include "rcc.h"
#include "keywords.h"

char *kw_canon[KW_COUNT + 1];
unsigned kw_flags[KW_COUNT + 1];

void init_keywords(void) {
    static bool done = false;
    if (done)
        return;
    done = true;

    for (int i = 0; i <= MAX_HASH_VALUE; i++) {
        const struct keyword_entry *e = &keyword_table[i];
        if (e->name) {
            kw_canon[e->id] = str_intern(e->name, strlen(e->name));
            kw_flags[e->id] = e->flags;
        }
    }
}

int keyword_id(const char *str, size_t len, char **interned_out) {
    const struct keyword_entry *e = keyword_lookup(str, len);
    if (!e)
        return ID_NONE;
    if (interned_out)
        *interned_out = kw_canon[e->id];
    return e->id;
}

char *keyword_interned(const char *str, size_t len) {
    const struct keyword_entry *e = keyword_lookup(str, len);
    return e ? kw_canon[e->id] : NULL;
}
