CC      = gcc
CFLAGS  = -std=c11 -O2 -Wall -Wextra
LDLIBS  = -lm
LIB     = movie.o hash.o heap.o sort.o

movierank: main.o $(LIB)
	$(CC) $(CFLAGS) -o $@ main.o $(LIB) $(LDLIBS)

bench: bench.o $(LIB)
	$(CC) $(CFLAGS) -o $@ bench.o $(LIB) $(LDLIBS)

stress: stress.o $(LIB)
	$(CC) $(CFLAGS) -o $@ stress.o $(LIB) $(LDLIBS)

%.o: %.c movie.h hash.h heap.h sort.h
	$(CC) $(CFLAGS) -c $<

run: movierank
	./movierank movies.csv

# Correctness check: 30,000 random rating updates compared against a full sort.
test: stress
	./stress movies.csv

# Timing experiments (quick mode, about a minute). Run ./bench for the full version.
bench-quick: bench
	./bench --quick

clean:
	rm -f *.o movierank bench stress
