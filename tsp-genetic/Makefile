LDFLAGS = -g -lpthread -lrt
CFLAGS = -g -Wpedantic -Wall -Wextra

binaries: tsp generate verify

.PHONY: plots
plots: binaries
	python make_plots.py

tsp: thread_pool.h maxrandom.h graph.h
tsp: tsp.o thread_pool.o maxrandom.o graph.o

generate: generate.o graph.o

verify: verify.o graph.o

graph.o: graph.h

thread_pool.o: thread_pool.h maxrandom.h

maxrandom.o: maxrandom.h

.PHONY: clean
clean:
	-rm tsp *.o
