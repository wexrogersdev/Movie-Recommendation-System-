/*
 * bench.c: timing experiments behind the "Experimental results" slides.
 *
 *   make bench-quick  quick run (1 timed trial per point, about 1 minute)
 *   make bench && ./bench   full run (best of 5 trials per point, several minutes)
 *
 * Prints, for random scores:
 *   1. time vs N for Min-Heap Top-K, Merge Sort, Quick Sort and brute force (K passes)
 *   2. heap best / random / worst case at N = 1,000,000
 *   3. heap replacements measured vs the prediction K * ln(N / K)
 *   4. quick sort on sorted vs random input
 *   5. time vs K at N = 100,000
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "movie.h"
#include "heap.h"
#include "sort.h"

#define K 10

static unsigned long long rng_state = 88172645463325252ULL;
static int trials = 5;

/* xorshift64: uniform double in [0, 5) */
static double rnd(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (double)(rng_state >> 11) * (5.0 / 9007199254740992.0);
}

static double now_ms(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec * 1e3 + (double)t.tv_nsec / 1e6;
}

/* mode 0: random scores, 1: best first (heap best case), 2: worst first (heap worst case) */
static void fill(Movie *m, size_t n, int mode)
{
    for (size_t i = 0; i < n; i++) {
        memset(&m[i], 0, sizeof(Movie));
        m[i].id = (int)i;
        m[i].heap_pos = -1;
        m[i].score = rnd();
        if (mode == 1) m[i].score = 10.0 - (double)i / (double)n * 5.0;
        if (mode == 2) m[i].score = 5.0 + (double)i / (double)n * 5.0;
    }
}

/* One heap Top-K pass; optionally counts how many movies replaced the root. */
static void heap_run(Movie *m, size_t n, int k, long *replacements)
{
    MinHeap h;
    long r = 0;
    heap_init(&h, k, 0);
    for (size_t i = 0; i < n; i++) {
        Movie *old_root = (h.size == k) ? h.a[0] : NULL;
        heap_offer(&h, &m[i]);
        if (old_root && h.a[0] != old_root) r++;
    }
    Movie **out = malloc((size_t)k * sizeof *out);
    heap_extract_sorted(&h, out);
    free(out);
    heap_free(&h);
    if (replacements) *replacements = r;
}

/* Brute force: k linear passes, each selecting the best remaining movie. */
static void brute_run(Movie *m, Movie **tmp, size_t n, int k)
{
    for (size_t i = 0; i < n; i++) tmp[i] = &m[i];
    for (int p = 0; p < k; p++) {
        size_t best = (size_t)p;
        for (size_t i = (size_t)p + 1; i < n; i++)
            if (movie_better(tmp[i], tmp[best])) best = i;
        Movie *t = tmp[p]; tmp[p] = tmp[best]; tmp[best] = t;
    }
}

static void sort_run(Movie *m, Movie **tmp, size_t n, int use_quick)
{
    for (size_t i = 0; i < n; i++) tmp[i] = &m[i];
    if (use_quick) quick_sort(tmp, n); else merge_sort(tmp, n);
}

typedef struct { Movie *m; Movie **tmp; size_t n; int k; } Ctx;
typedef void (*Job)(const Ctx *);

static void job_heap(const Ctx *c)  { heap_run(c->m, c->n, c->k, NULL); }
static void job_merge(const Ctx *c) { sort_run(c->m, c->tmp, c->n, 0); }
static void job_quick(const Ctx *c) { sort_run(c->m, c->tmp, c->n, 1); }
static void job_brute(const Ctx *c) { brute_run(c->m, c->tmp, c->n, c->k); }

/* Best (lowest) average time over `trials` trials of `reps` repetitions. */
static double measure(Job job, const Ctx *c, int reps)
{
    double best = 1e18;
    for (int t = 0; t < trials; t++) {
        double t0 = now_ms();
        for (int r = 0; r < reps; r++) job(c);
        double d = (now_ms() - t0) / reps;
        if (d < best) best = d;
    }
    return best;
}

int main(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "--quick") == 0) trials = 1;

    size_t sizes[] = {1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000};
    const int nsizes = (int)(sizeof sizes / sizeof sizes[0]);

    printf("# 1. Time per query in ms, K = %d, random scores (best of %d trials)\n", K, trials);
    printf("# N heap merge quick brute\n");
    for (int s = 0; s < nsizes; s++) {
        size_t n = sizes[s];
        Movie *m = malloc(n * sizeof(Movie));
        Movie **tmp = malloc(n * sizeof(Movie *));
        fill(m, n, 0);
        Ctx c = { m, tmp, n, K };
        int reps = (int)(2e7 / (double)n);
        if (reps < 5) reps = 5;
        if (reps > 2000) reps = 2000;
        printf("%zu %.5f %.5f %.5f %.5f\n", n,
               measure(job_heap, &c, reps), measure(job_merge, &c, reps),
               measure(job_quick, &c, reps), measure(job_brute, &c, reps));
        fflush(stdout);
        free(m);
        free(tmp);
    }

    size_t n = 1000000;
    Movie *m = malloc(n * sizeof(Movie));
    Movie **tmp = malloc(n * sizeof(Movie *));
    long repl = 0;

    printf("\n# 2. Heap cases at N = %zu: case time_ms replacements\n", n);
    const char *names[] = {"random", "best", "worst"};
    for (int mode = 0; mode < 3; mode++) {
        fill(m, n, mode);
        double t0 = now_ms();
        for (int r = 0; r < 10; r++) heap_run(m, n, K, &repl);
        printf("%s %.3f %ld\n", names[mode], (now_ms() - t0) / 10, repl);
    }

    printf("\n# 3. Replacements, mean of 20 random trials: N measured predicted K*ln(N/K)\n");
    size_t rn[] = {1000, 10000, 100000, 1000000};
    for (int i = 0; i < 4; i++) {
        double sum = 0;
        for (int t = 0; t < 20; t++) { fill(m, rn[i], 0); heap_run(m, rn[i], K, &repl); sum += (double)repl; }
        printf("%zu %.1f %.1f\n", rn[i], sum / 20.0, K * (log((double)rn[i]) - log((double)K)));
    }

    printf("\n# 4. Quick sort at N = %zu, sorted vs random input (ms)\n", n);
    fill(m, n, 1);
    sort_run(m, tmp, n, 0);          /* tmp is now sorted best-first */
    double t0 = now_ms();
    quick_sort(tmp, n);
    printf("sorted %.3f\n", now_ms() - t0);
    fill(m, n, 0);
    for (size_t i = 0; i < n; i++) tmp[i] = &m[i];
    t0 = now_ms();
    quick_sort(tmp, n);
    printf("random %.3f\n", now_ms() - t0);
    free(m);
    free(tmp);

    printf("\n# 5. Time vs K at N = 100000 (ms): K heap brute merge\n");
    n = 100000;
    m = malloc(n * sizeof(Movie));
    tmp = malloc(n * sizeof(Movie *));
    fill(m, n, 0);
    int ks[] = {1, 10, 100, 1000};
    for (int i = 0; i < 4; i++) {
        Ctx c = { m, tmp, n, ks[i] };
        double th = measure(job_heap, &c, 5);
        double tb = measure(job_brute, &c, ks[i] <= 100 ? 4 : 1);
        double tm = measure(job_merge, &c, 3);
        printf("%d %.4f %.4f %.4f\n", ks[i], th, tb, tm);
        fflush(stdout);
    }
    free(m);
    free(tmp);
    return 0;
}
