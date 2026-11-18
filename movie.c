#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "movie.h"

double movie_avg(const Movie *m)
{
    return m->rating_count > 0 ? m->rating_sum / (double)m->rating_count : 0.0;
}

/* Weighted rating: a movie with only 2 votes at 5.0 should not beat one with
 * thousands of votes at 4.6. Views add a small log bonus. */
double movie_score(const Catalogue *c, const Movie *m)
{
    double v  = (double)m->rating_count;
    double r  = movie_avg(m);
    double wr = (v / (v + MIN_VOTES)) * r + (MIN_VOTES / (v + MIN_VOTES)) * c->global_mean;
    return wr + VIEW_WEIGHT * log10(1.0 + (double)m->views);
}

long load_catalogue(const char *path, Catalogue *c)
{
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    size_t cap = 1024;
    c->items = malloc(cap * sizeof(Movie));
    c->count = 0;
    c->global_mean = 0.0;
    if (!c->items) { fclose(f); return -1; }

    char line[512];
    long skipped = 0;
    double sum = 0.0;
    double votes = 0.0;

    if (!fgets(line, sizeof line, f)) { fclose(f); return 0; } /* skip header */

    while (fgets(line, sizeof line, f)) {
        int id, year;
        char title[TITLE_LEN], genre[GENRE_LEN];
        double avg;
        long cnt, views;

        int n = sscanf(line, "%d,%63[^,],%15[^,],%d,%lf,%ld,%ld",
                       &id, title, genre, &year, &avg, &cnt, &views);
        if (n != 7 || cnt < 0 || views < 0) { skipped++; continue; }

        if (c->count == cap) {
            cap *= 2;
            Movie *bigger = realloc(c->items, cap * sizeof(Movie));
            if (!bigger) { fclose(f); return -1; }
            c->items = bigger;
        }

        Movie *m = &c->items[c->count++];
        memset(m, 0, sizeof *m);
        m->id = id;
        strcpy(m->title, title);
        strcpy(m->genre, genre);
        m->year = year;
        m->rating_count = cnt;
        m->rating_sum = avg * (double)cnt;
        m->views = views;
        m->heap_pos = -1;

        sum += m->rating_sum;
        votes += (double)cnt;
    }
    fclose(f);

    c->global_mean = votes > 0 ? sum / votes : 3.0;
    for (size_t i = 0; i < c->count; i++)
        c->items[i].score = movie_score(c, &c->items[i]);

    if (skipped > 0)
        fprintf(stderr, "Warning: skipped %ld malformed row(s).\n", skipped);
    return (long)c->count;
}

void free_catalogue(Catalogue *c)
{
    free(c->items);
    c->items = NULL;
    c->count = 0;
}
