#include <stdlib.h>
#include <string.h>
#include "heap.h"
#include "sort.h"

static void place(MinHeap *h, int i, Movie *m)
{
    h->a[i] = m;
    if (h->track) m->heap_pos = i;
}

static void sift_up(MinHeap *h, int i)
{
    Movie *m = h->a[i];
    while (i > 0) {
        int p = (i - 1) / 2;
        if (!movie_better(h->a[p], m)) break;
        place(h, i, h->a[p]);
        i = p;
    }
    place(h, i, m);
}

static void sift_down(MinHeap *h, int i)
{
    Movie *m = h->a[i];
    for (;;) {
        int c = 2 * i + 1;                       /* left child */
        if (c >= h->size) break;
        if (c + 1 < h->size && movie_better(h->a[c], h->a[c + 1])) c++;   /* pick worse child */
        if (movie_better(h->a[c], m)) break;
        place(h, i, h->a[c]);
        i = c;
    }
    place(h, i, m);
}

int heap_init(MinHeap *h, int k, int track)
{
    h->a = malloc((size_t)(k > 0 ? k : 1) * sizeof(Movie *));
    h->size = 0;
    h->cap = k;
    h->track = track;
    return h->a ? 0 : -1;
}

void heap_free(MinHeap *h)
{
    free(h->a);
    h->a = NULL;
    h->size = h->cap = 0;
}

void heap_clear(MinHeap *h)
{
    if (h->track)
        for (int i = 0; i < h->size; i++) h->a[i]->heap_pos = -1;
    h->size = 0;
}

void heap_offer(MinHeap *h, Movie *m)
{
    if (h->cap <= 0) return;
    if (h->size < h->cap) {                      /* heap not full yet */
        h->a[h->size++] = m;
        sift_up(h, h->size - 1);
    } else if (movie_better(m, h->a[0])) {       /* better than the current worst */
        if (h->track) h->a[0]->heap_pos = -1;
        h->a[0] = m;
        sift_down(h, 0);
    }
}

int heap_fix(MinHeap *h, Movie *m, double old_score)
{
    if (!h->track || m->heap_pos < 0) return 0;
    if (m->score > old_score) { sift_down(h, m->heap_pos); return 0; }
    if (m->score < old_score) { sift_up(h, m->heap_pos);   return 1; }
    return 0;
}

int heap_extract_sorted(const MinHeap *h, Movie **out)
{
    memcpy(out, h->a, (size_t)h->size * sizeof(Movie *));
    merge_sort(out, (size_t)h->size);
    return h->size;
}
