LDFLAGS = -lrt
CFLAGS = -O3

binaries: random_walk_omp random_walk_simple

.PHONY: plots
plots: binaries
	python make_plots.py

random_walk_omp: LDFLAGS += -fopenmp
random_walk_omp: random_walk.o random_walk.h

random_walk_simple: random_walk.o random_walk.h

.PHONY: clean
clean:
	-rm random_walk_omp random_walk_simple *.o
