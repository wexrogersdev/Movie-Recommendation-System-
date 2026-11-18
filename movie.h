#ifndef MOVIE_H
#define MOVIE_H

#include <stddef.h>

#define TITLE_LEN 64
#define GENRE_LEN 16

#define MIN_VOTES   50.0   /* votes needed before a movie's own average matters */
#define VIEW_WEIGHT 0.10   /* bonus for views */

typedef struct Movie {
    int    id;
    char   title[TITLE_LEN];
    char   genre[GENRE_LEN];
    int    year;
    double rating_sum;     /* sum of ratings, so adding a new one is O(1) */
    long   rating_count;
    long   views;
    double score;
    int    heap_pos;       /* position in the trending heap, -1 if not in it */
} Movie;

typedef struct {
    Movie  *items;
    size_t  count;
    double  global_mean;   /* average rating of all movies */
} Catalogue;

long   load_catalogue(const char *path, Catalogue *c);   /* returns movies loaded, -1 on error */
void   free_catalogue(Catalogue *c);
double movie_avg(const Movie *m);
double movie_score(const Catalogue *c, const Movie *m);

/* One ordering for everything: higher score first, lower id if equal. */
static inline int movie_better(const Movie *a, const Movie *b)
{
    if (a->score != b->score) return a->score > b->score;
    return a->id < b->id;
}

#endif
