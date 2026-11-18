#include <stdlib.h>
#include "hash.h"

/* Mixes the bits of the id so nearby ids spread out across the table. */
static size_t hash_id(int id, size_t cap)
{
    unsigned int x = (unsigned int)id;
    x ^= x >> 16;
    x *= 0x45d9f3bu;
    x ^= x >> 16;
    x *= 0x45d9f3bu;
    x ^= x >> 16;
    return (size_t)x & (cap - 1);
}

int ht_init(HashTable *t, size_t expected)
{
    size_t cap = 16;
    while (cap < expected * 2) cap <<= 1;   /* load factor stays <= 0.5 */
    t->slots = calloc(cap, sizeof(Movie *));
    t->cap = cap;
    t->size = 0;
    return t->slots ? 0 : -1;
}

int ht_insert(HashTable *t, Movie *m)
{
    if (t->size >= t->cap) return -1;
    size_t i = hash_id(m->id, t->cap);
    while (t->slots[i]) {
        if (t->slots[i]->id == m->id) return -1;   /* duplicate */
        i = (i + 1) & (t->cap - 1);                /* next slot */
    }
    t->slots[i] = m;
    t->size++;
    return 0;
}

Movie *ht_get(const HashTable *t, int id)
{
    size_t i = hash_id(id, t->cap);
    while (t->slots[i]) {
        if (t->slots[i]->id == id) return t->slots[i];
        i = (i + 1) & (t->cap - 1);
    }
    return NULL;
}

void ht_free(HashTable *t)
{
    free(t->slots);
    t->slots = NULL;
    t->cap = t->size = 0;
}
