#include <stdlib.h>
#include <string.h>
#include "sort.h"

/* ---------- Merge sort ---------- */

static void msort(Movie **a, Movie **tmp, size_t lo, size_t hi)   /* sorts a[lo..hi) */
{
    if (hi - lo < 2) return;
    size_t mid = lo + (hi - lo) / 2;
    msort(a, tmp, lo, mid);
    msort(a, tmp, mid, hi);

    size_t i = lo, j = mid, k = lo;                 /* merge the two halves */
    while (i < mid && j < hi)
        tmp[k++] = movie_better(a[j], a[i]) ? a[j++] : a[i++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    memcpy(a + lo, tmp + lo, (hi - lo) * sizeof(Movie *));
}

void merge_sort(Movie **a, size_t n)
{
    if (n < 2) return;
    Movie **tmp = malloc(n * sizeof(Movie *));
    if (!tmp) return;
    msort(a, tmp, 0, n);
    free(tmp);
}

/* ---------- Quick sort ---------- */

static void swap_p(Movie **x, Movie **y)
{
    Movie *t = *x;
    *x = *y;
    *y = t;
}

static void insertion(Movie **a, long lo, long hi)   /* sorts a[lo..hi] */
{
    for (long i = lo + 1; i <= hi; i++) {
        Movie *x = a[i];
        long j = i - 1;
        while (j >= lo && movie_better(x, a[j])) {
            a[j + 1] = a[j];
            j--;
        }
        a[j + 1] = x;
    }
}

static void qsort_range(Movie **a, long lo, long hi)
{
    while (lo < hi) {
        if (hi - lo < 16) { insertion(a, lo, hi); return; }   /* small part */

        /* pivot = median of first, middle, last */
        long mid = lo + (hi - lo) / 2;
        if (movie_better(a[mid], a[lo]))  swap_p(&a[mid], &a[lo]);
        if (movie_better(a[hi],  a[lo]))  swap_p(&a[hi],  &a[lo]);
        if (movie_better(a[hi],  a[mid])) swap_p(&a[hi],  &a[mid]);
        swap_p(&a[mid], &a[hi - 1]);
        Movie *pivot = a[hi - 1];

        long i = lo, j = hi - 1;
        for (;;) {
            while (movie_better(a[++i], pivot)) ;
            while (movie_better(pivot, a[--j])) ;
            if (i >= j) break;
            swap_p(&a[i], &a[j]);
        }
        swap_p(&a[i], &a[hi - 1]);

        /* recurse on the smaller side, loop on the bigger side (small stack) */
        if (i - lo < hi - i) { qsort_range(a, lo, i - 1); lo = i + 1; }
        else                 { qsort_range(a, i + 1, hi); hi = i - 1; }
    }
}

void quick_sort(Movie **a, size_t n)
{
    if (n < 2) return;
    qsort_range(a, 0, (long)n - 1);
}
