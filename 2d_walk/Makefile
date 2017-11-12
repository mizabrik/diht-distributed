CC=mpicc
LDFLAGS = -g
CFLAGS = -g --pedantic -Wall -Wextra

binaries: 2d_walk

2d_walk: send_queue.h 2d_walk.h 2d_walk.o send_queue.o

send_queue.o: send_queue.h

.PHONY: clean
clean:
	-rm 2d_walk *.o
