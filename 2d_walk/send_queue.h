#ifndef SEND_QUEUE_H_
#define SEND_QUEUE_H_

#include "2d_walk.h"

typedef struct {
  int worker_count;
  int buf_size;

  int *count;
  int *capacity;
  particle_t **bufs;

  int *busy_capacity;
  particle_t **busy_bufs;

  int *busy;
  MPI_Request *requests;
} send_queue_t;

void send_queue_init(send_queue_t *q, int worker_count, int buf_size);
void send_queue_add(send_queue_t *q, int dest, particle_t p);
void send_queue_send(send_queue_t *q);
void send_queue_destroy(send_queue_t *q);

#endif /* SEND_QUEUE_H_ */
