#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include "movie.h"

/* Hash table (linear probing): movie ID -> Movie. Average O(1) lookup. */
typedef struct {
    Movie **slots;
    size_t  cap;    /* power of two */
    size_t  size;
} HashTable;

int    ht_init(HashTable *t, size_t expected);   /* 0 on success */
int    ht_insert(HashTable *t, Movie *m);        /* 0 ok, -1 duplicate id or full */
Movie *ht_get(const HashTable *t, int id);       /* NULL if not found */
void   ht_free(HashTable *t);

#endif
