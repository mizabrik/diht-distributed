LDFLAGS = -g -lpthread -lrt
CFLAGS = -g -Wpedantic -Wall -Wextra

binaries: sort

.PHONY: plots
plots: binaries
	python make_plots.py

sort: sort.o thread_pool.o

thread_pool.o: thread_pool.h

.PHONY: clean
clean:
	-rm sort *.o
