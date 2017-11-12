CC=mpicc
LDFLAGS = -g
CFLAGS = -g --pedantic -Wall -Wextra

binaries: io verify

io: io.o

verify: CC = gcc
verify: verify.o

.PHONY: clean
clean:
	-rm io verify *.o
