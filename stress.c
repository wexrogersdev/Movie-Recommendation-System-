/*
 * stress.c: correctness test for the live-update logic.
 *
 *   make test
 *
 * Applies 30,000 random rating updates to the trending heap, exactly as the
 * menu's "Submit a rating" does, and every 500 steps checks that:
 *   - the heap's Top 10 equals the Top 10 from a full merge sort,
 *   - quick sort and merge sort order the whole catalogue identically,
 *   - every movie in the heap has a consistent heap_pos.
 * Exit code 0 means no mismatches. Build with -fsanitize=address,undefined for extra checks.
 */
#include <stdio.h>
#include <stdlib.h>
#include "movie.h"
#include "hash.h"
#include "heap.h"
#include "sort.h"

#define K 10

static void rebuild(Catalogue *c, MinHeap *h)
{
    heap_clear(h);
    for (size_t i = 0; i < c->count; i++) heap_offer(h, &c->items[i]);
}

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "movies.csv";
    Catalogue c;
    if (load_catalogue(path, &c) <= 0) {
        fprintf(stderr, "Could not load %s\n", path);
        return 2;
    }

    HashTable ht;
    ht_init(&ht, c.count);
    for (size_t i = 0; i < c.count; i++) ht_insert(&ht, &c.items[i]);

    MinHeap h;
    heap_init(&h, K, 1);
    rebuild(&c, &h);

    Movie **all = malloc(c.count * sizeof *all);
    Movie **quick = malloc(c.count * sizeof *quick);
    int mismatches = 0, rebuilds = 0, checks = 0;
    srand(7);

    for (int step = 0; step < 30000; step++) {
        /* every third update hits a movie that is currently trending */
        Movie *m = (step % 3 == 0 && h.size > 0) ? h.a[rand() % h.size]
                                                  : &c.items[rand() % c.count];
        Movie *found = ht_get(&ht, m->id);        /* exercise the hash table too */
        if (found != m) mismatches++;

        double old = m->score;
        m->rating_sum += 1 + rand() % 5;
        m->rating_count++;
        m->score = movie_score(&c, m);

        if (m->heap_pos >= 0) {
            if (heap_fix(&h, m, old)) { rebuild(&c, &h); rebuilds++; }
        } else {
            heap_offer(&h, m);
        }

        if (step % 500 == 0) {
            checks++;
            for (size_t i = 0; i < c.count; i++) { all[i] = &c.items[i]; quick[i] = &c.items[i]; }
            merge_sort(all, c.count);
            quick_sort(quick, c.count);
            Movie *top[K];
            int n = heap_extract_sorted(&h, top);
            for (int k = 0; k < n; k++) if (top[k]->id != all[k]->id) mismatches++;
            for (size_t i = 0; i < c.count; i++) if (quick[i]->id != all[i]->id) { mismatches++; break; }
        }
    }
    for (int i = 0; i < h.size; i++) if (h.a[i]->heap_pos != i) mismatches++;

    printf("%d checks, %d rebuilds, %d mismatches\n", checks, rebuilds, mismatches);

    free(all);
    free(quick);
    heap_free(&h);
    ht_free(&ht);
    free_catalogue(&c);
    return mismatches != 0;
}
