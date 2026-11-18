/*
 * Movie Recommendation Ranking System
 * Console app: Min-Heap (Top K), Merge/Quick Sort (browse), Hash Table (lookup).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "movie.h"
#include "hash.h"
#include "heap.h"
#include "sort.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#define TRENDING_K  10
#define MAX_GENRES  32
#define BENCH_REPS  200

static int g_eof = 0;

/* ---------- input ---------- */

static void read_line(const char *prompt, char *buf, size_t n)
{
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, (int)n, stdin)) { g_eof = 1; buf[0] = '\0'; return; }
    buf[strcspn(buf, "\r\n")] = '\0';
}

static long read_long(const char *prompt, long fallback)
{
    char buf[64];
    read_line(prompt, buf, sizeof buf);
    if (buf[0] == '\0') return fallback;
    char *end;
    long v = strtol(buf, &end, 10);
    return (end == buf) ? fallback : v;
}

static double read_double(const char *prompt, double fallback)
{
    char buf[64];
    read_line(prompt, buf, sizeof buf);
    if (buf[0] == '\0') return fallback;
    char *end;
    double v = strtod(buf, &end);
    return (end == buf) ? fallback : v;
}

/* ---------- display ---------- */

static void print_movies(Movie **list, int n)
{
    if (n == 0) { puts("  (no movies)"); return; }
    printf("  %-4s %-6s %-30s %-9s %-5s %-6s %-7s %-9s %s\n",
           "Rank", "ID", "Title", "Genre", "Year", "Avg", "Votes", "Views", "Score");
    for (int i = 0; i < n; i++) {
        const Movie *m = list[i];
        printf("  %-4d %-6d %-30.30s %-9s %-5d %-6.2f %-7ld %-9ld %.3f\n",
               i + 1, m->id, m->title, m->genre, m->year,
               movie_avg(m), m->rating_count, m->views, m->score);
    }
}

static int collect_genres(const Catalogue *c, char genres[][GENRE_LEN])
{
    int n = 0;
    for (size_t i = 0; i < c->count && n < MAX_GENRES; i++) {
        int seen = 0;
        for (int g = 0; g < n; g++)
            if (strcasecmp(genres[g], c->items[i].genre) == 0) { seen = 1; break; }
        if (!seen) strcpy(genres[n++], c->items[i].genre);
    }
    return n;
}

static void print_genres(int n, char genres[][GENRE_LEN])
{
    printf("\nGenres:");
    for (int i = 0; i < n; i++) printf(" %s%s", genres[i], i + 1 < n ? "," : "");
    putchar('\n');
}

/* ---------- ranking ---------- */

/* Top K of all movies, or of one genre if genre is not NULL.
 * One pass with a size-K min-heap: O(N log K), no full sort. */
static int top_k(const Catalogue *c, const char *genre, int k, Movie **out)
{
    MinHeap h;
    if (heap_init(&h, k, 0) != 0) return 0;
    for (size_t i = 0; i < c->count; i++) {
        Movie *m = &c->items[i];
        if (genre && strcasecmp(m->genre, genre) != 0) continue;
        heap_offer(&h, m);
    }
    int n = heap_extract_sorted(&h, out);
    heap_free(&h);
    return n;
}

static void rebuild_trending(Catalogue *c, MinHeap *trending)
{
    heap_clear(trending);
    for (size_t i = 0; i < c->count; i++) heap_offer(trending, &c->items[i]);
}

static double elapsed_ms(clock_t start)
{
    return (double)(clock() - start) * 1000.0 / (double)CLOCKS_PER_SEC;
}

/* ---------- menu options ---------- */

static void show_trending(const MinHeap *trending)
{
    Movie *list[TRENDING_K];
    int n = heap_extract_sorted(trending, list);
    printf("\nTop %d trending movies (from the live heap):\n", TRENDING_K);
    print_movies(list, n);
}

static void show_genre_top_k(const Catalogue *c, int ngenres, char genres[][GENRE_LEN])
{
    char genre[GENRE_LEN];
    print_genres(ngenres, genres);
    read_line("Genre: ", genre, sizeof genre);
    long k = read_long("How many (default 5): ", 5);
    if (k < 1) k = 5;
    if (k > 100) k = 100;

    Movie **out = malloc((size_t)k * sizeof(Movie *));
    if (!out) return;
    clock_t t0 = clock();
    int n = top_k(c, genre, (int)k, out);
    double ms = elapsed_ms(t0);
    printf("\nTop %d %s movies (%.3f ms):\n", (int)k, genre, ms);
    print_movies(out, n);
    free(out);
}

static void browse_genre(const Catalogue *c, int ngenres, char genres[][GENRE_LEN])
{
    char genre[GENRE_LEN];
    print_genres(ngenres, genres);
    read_line("Genre to browse: ", genre, sizeof genre);
    long algo  = read_long("Sort with 1) Merge Sort  2) Quick Sort (default 1): ", 1);
    long limit = read_long("How many to display (default 20): ", 20);
    if (limit < 1) limit = 20;

    Movie **list = malloc(c->count * sizeof(Movie *));
    if (!list) return;
    size_t n = 0;
    for (size_t i = 0; i < c->count; i++)
        if (strcasecmp(c->items[i].genre, genre) == 0) list[n++] = &c->items[i];

    clock_t t0 = clock();
    if (algo == 2) quick_sort(list, n); else merge_sort(list, n);
    double ms = elapsed_ms(t0);

    printf("\n%s: %zu movies sorted with %s in %.3f ms, showing %ld:\n",
           genre, n, algo == 2 ? "Quick Sort" : "Merge Sort", ms, limit);
    print_movies(list, (int)(n < (size_t)limit ? n : (size_t)limit));
    free(list);
}

static void lookup_movie(const HashTable *ht)
{
    int id = (int)read_long("Movie ID: ", -1);
    Movie *m = ht_get(ht, id);
    if (!m) { puts("No movie with that ID."); return; }
    printf("\n  [%d] %s (%d)\n  Genre: %s | Avg rating: %.2f from %ld votes | Views: %ld | Score: %.3f\n",
           m->id, m->title, m->year, m->genre, movie_avg(m), m->rating_count, m->views, m->score);
}

static void submit_rating(Catalogue *c, const HashTable *ht, MinHeap *trending)
{
    int id = (int)read_long("Movie ID: ", -1);
    Movie *m = ht_get(ht, id);
    if (!m) { puts("No movie with that ID."); return; }
    double r = read_double("Your rating (1-5): ", 0.0);
    if (r < 1.0 || r > 5.0) { puts("Rating must be between 1 and 5."); return; }

    double old = m->score;
    int was_trending = m->heap_pos >= 0;

    m->rating_sum += r;
    m->rating_count++;
    m->score = movie_score(c, m);        /* only this movie changes */

    const char *note;
    if (was_trending) {
        if (heap_fix(trending, m, old)) {
            rebuild_trending(c, trending);
            note = "score dropped, so the trending list was rebuilt (O(N log K))";
        } else {
            note = "score rose or held, so the heap was adjusted in place (O(log K))";
        }
    } else {
        heap_offer(trending, m);          /* one check against the root */
        note = m->heap_pos >= 0 ? "it now ENTERS the Top 10 (O(log K))"
                                : "checked against the heap root: still outside the Top 10 (O(1))";
    }
    printf("\n  \"%s\": score %.3f -> %.3f. %s\n", m->title, old, m->score, note);
}

static int same_ids(Movie **a, Movie **b, int n)
{
    for (int i = 0; i < n; i++) if (a[i]->id != b[i]->id) return 0;
    return 1;
}

static void benchmark(Catalogue *c)
{
    Movie *r_heap[TRENDING_K], *r_merge[TRENDING_K], *r_quick[TRENDING_K];
    Movie **tmp = malloc(c->count * sizeof(Movie *));
    if (!tmp) return;
    int n = 0;
    clock_t t0;

    t0 = clock();
    for (int rep = 0; rep < BENCH_REPS; rep++) n = top_k(c, NULL, TRENDING_K, r_heap);
    double heap_ms = elapsed_ms(t0) / BENCH_REPS;

    t0 = clock();
    for (int rep = 0; rep < BENCH_REPS; rep++) {
        for (size_t i = 0; i < c->count; i++) tmp[i] = &c->items[i];
        merge_sort(tmp, c->count);
    }
    double merge_ms = elapsed_ms(t0) / BENCH_REPS;
    memcpy(r_merge, tmp, sizeof r_merge);

    t0 = clock();
    for (int rep = 0; rep < BENCH_REPS; rep++) {
        for (size_t i = 0; i < c->count; i++) tmp[i] = &c->items[i];
        quick_sort(tmp, c->count);
    }
    double quick_ms = elapsed_ms(t0) / BENCH_REPS;
    memcpy(r_quick, tmp, sizeof r_quick);

    printf("\nBenchmark: Top %d of %zu movies, averaged over %d runs\n", TRENDING_K, c->count, BENCH_REPS);
    printf("  Min-Heap (one pass, size K)    %8.4f ms/query\n", heap_ms);
    printf("  Merge Sort whole dataset       %8.4f ms/query\n", merge_ms);
    printf("  Quick Sort whole dataset       %8.4f ms/query\n", quick_ms);
    printf("  Heap is %.1fx faster than a full merge sort\n", heap_ms > 0 ? merge_ms / heap_ms : 0.0);
    printf("  All three give the same Top %d: %s\n", TRENDING_K,
           (same_ids(r_heap, r_merge, n) && same_ids(r_heap, r_quick, n)) ? "yes" : "NO (bug!)");
    free(tmp);
}

/* ---------- main ---------- */

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "movies.csv";

    Catalogue cat;
    long loaded = load_catalogue(path, &cat);
    if (loaded < 0) {
        fprintf(stderr, "Could not open '%s'. Usage: %s [movies.csv]\n", path, argv[0]);
        return 1;
    }
    if (loaded == 0) {
        fprintf(stderr, "'%s' contains no movies.\n", path);
        return 1;
    }

    HashTable ht;
    if (ht_init(&ht, cat.count) != 0) { fputs("Out of memory.\n", stderr); return 1; }
    long dupes = 0;
    for (size_t i = 0; i < cat.count; i++)
        if (ht_insert(&ht, &cat.items[i]) != 0) dupes++;
    if (dupes) fprintf(stderr, "Warning: %ld duplicate movie ID(s) ignored in the lookup table.\n", dupes);

    MinHeap trending;
    if (heap_init(&trending, TRENDING_K, 1) != 0) { fputs("Out of memory.\n", stderr); return 1; }
    rebuild_trending(&cat, &trending);

    char genres[MAX_GENRES][GENRE_LEN];
    int ngenres = collect_genres(&cat, genres);

    printf("Loaded %zu movies from %s (global mean rating %.2f)\n", cat.count, path, cat.global_mean);

    while (!g_eof) {
        puts("\n=== Movie Recommendation Ranking System ===");
        puts("1) Top 10 trending movies");
        puts("2) Top K in a genre");
        puts("3) Browse a genre, fully sorted");
        puts("4) Look up a movie by ID");
        puts("5) Submit a rating");
        puts("6) Benchmark: heap vs full sort");
        puts("0) Exit");
        long choice = read_long("Choose: ", -1);
        if (g_eof) break;

        switch (choice) {
        case 1: show_trending(&trending); break;
        case 2: show_genre_top_k(&cat, ngenres, genres); break;
        case 3: browse_genre(&cat, ngenres, genres); break;
        case 4: lookup_movie(&ht); break;
        case 5: submit_rating(&cat, &ht, &trending); break;
        case 6: benchmark(&cat); break;
        case 0: g_eof = 1; break;
        default: puts("Please choose 0-6."); break;
        }
    }

    heap_free(&trending);
    ht_free(&ht);
    free_catalogue(&cat);
    puts("Goodbye.");
    return 0;
}
