# Movie Recommendation Ranking System

A console app in C that ranks movies with a Min-Heap, Merge/Quick Sort and a Hash Table.
Data lives in memory, loaded from `movies.csv` (5,000 generated movies, 7 genres).

## Build and run

```
make
./movierank movies.csv
```

Needs `gcc` and `make` (Linux, macOS, or MinGW/WSL on Windows). No other dependencies.

## Menu

| Option | What it does | Data structure | Cost |
| --- | --- | --- | --- |
| 1 | Top 10 trending movies, kept live | Min-Heap (size K) | read the heap, O(K log K) |
| 2 | Top K in a genre | Min-Heap, one pass | O(N log K) |
| 3 | Browse a genre fully sorted (choose Merge or Quick Sort) | Merge / Quick Sort | O(N log N) |
| 4 | Look up a movie by ID | Hash Table | O(1) average |
| 5 | Submit a rating and watch the ranking update | Heap + Hash Table | O(log K) or O(N log K) |
| 6 | Benchmark: heap vs full sort | all | prints measured times |

## Files

| File | Role |
| --- | --- |
| `movie.h`, `movie.c` | `Movie` struct, CSV loader, score formula |
| `hash.h`, `hash.c` | Array-based hash table (open addressing, linear probing, custom integer hash) |
| `heap.h`, `heap.c` | Bounded min-heap for Top K, with in-place update |
| `sort.h`, `sort.c` | Merge sort (stable) and quick sort (median-of-three) |
| `main.c` | Console menu |
| `stress.c` | Correctness test: 30,000 random rating updates checked against a full sort (`make test`) |
| `bench.c` | Timing experiments: N vs time, heap best/average/worst case, K sweep (`make bench-quick`) |
| `movies.csv` | Sample data: `id,title,genre,year,avg_rating,rating_count,views` |

## How scoring works

Each movie gets `score = weighted_rating + 0.10 * log10(1 + views)`.

`weighted_rating = (v / (v + 50)) * R + (50 / (v + 50)) * C`, where `R` is the movie's average,
`v` its number of votes and `C` the catalogue-wide mean. A movie with few votes is pulled toward
the mean, so two votes at 5.0 do not outrank thousands of votes at 4.6. Change `MIN_VOTES` and
`VIEW_WEIGHT` in `movie.h` to tune it.

## How live updates work

Submitting a rating recalculates only that movie's score, then:

- **Movie not in the Top 10:** one comparison against the heap root. If it beats the root it
  replaces it (O(log K)).
- **Movie in the Top 10, score rose:** it sifts down in place (O(log K)). Nothing outside the
  heap can be affected.
- **Movie in the Top 10, score dropped:** a movie outside the heap may now deserve its seat, and a
  min-heap cannot tell which. The trending list is rebuilt in one pass (O(N log K)).

The global mean `C` is fixed at load time, so one rating never changes every other movie's score.

## Testing done

- Compiled with `-Wall -Wextra`, no warnings.
- Stress test (`make test`): 30,000 random rating updates (about 8,000 triggering a rebuild), checked every 500
  steps against a full merge sort. Top 10 matched every time, quick sort matched merge sort, and heap
  positions stayed consistent. Run under AddressSanitizer and UndefinedBehaviorSanitizer with no errors.
- Option 6 repeats the comparison on every run and prints whether all three methods agree.

## Ideas to extend

Trie for title autocomplete, collaborative filtering from per-user watch history, regional or
recency-based scores (add a field to `Movie` and a term to `movie_score`).

## Reproducing the experiments

```
make bench-quick     # about a minute, one timed trial per point
make bench && ./bench   # full run, best of 5 trials (several minutes)
```

Absolute times depend on the machine (the reported numbers came from a 2-vCPU Intel Xeon 2.3 GHz VM and varied by roughly 20% between runs). The shape of the curves and the gaps between methods are what to compare. All three Top-K methods are checked against each other in menu option 6.
