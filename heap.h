#ifndef HEAP_H
#define HEAP_H

#include "movie.h"

/* Min-heap of size K that keeps the K best movies seen so far.
 * The root is the worst of them, so a new movie is only compared with the root. */
typedef struct {
    Movie **a;
    int     size;
    int     cap;     /* K */
    int     track;   /* 1 = keep Movie.heap_pos updated */
} MinHeap;

int  heap_init(MinHeap *h, int k, int track);   /* 0 on success */
void heap_free(MinHeap *h);
void heap_clear(MinHeap *h);
void heap_offer(MinHeap *h, Movie *m);          /* O(log K) */

/* Call after a movie's score changed. Returns 1 if the score dropped
 * (caller must rebuild the heap), otherwise 0. */
int  heap_fix(MinHeap *h, Movie *m, double old_score);

/* Copies the heap into out[], best first. Returns how many. */
int  heap_extract_sorted(const MinHeap *h, Movie **out);

#endif
