#ifndef SORT_H
#define SORT_H

#include <stddef.h>
#include "movie.h"

/* Sort an array of Movie pointers, best first. Both are O(N log N) on average. */
void merge_sort(Movie **a, size_t n);   /* needs O(N) extra space */
void quick_sort(Movie **a, size_t n);   /* in place, median-of-three pivot */

#endif
