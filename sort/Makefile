LDFLAGS = -g -fopenmp -lrt
CFLAGS = -g -fopenmp -Wpedantic -Wall -Wextra

binaries: sort

.PHONY: plots
plots: binaries
	python make_plots.py

sort: sort.o

.PHONY: clean
clean:
	-rm sort *.o
