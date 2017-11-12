#include "send_queue.h"

#include <stdlib.h>

#include <mpi.h>

#include "2d_walk.h"

void send_queue_init(send_queue_t *q, int worker_count, int buf_size) {
  int i;

  q->worker_count = worker_count;
  q->buf_size = buf_size;

  q->count = (int *) calloc(worker_count, sizeof(int));
  q->capacity = (int *) calloc(worker_count, sizeof(int));
  q->bufs = (particle_t **) malloc(worker_count * sizeof(particle_t *));

  q->busy_capacity = (int *) calloc(worker_count, sizeof(int));
  q->busy_bufs = (particle_t **) malloc(worker_count * sizeof(particle_t *));

  q->busy = (int *) calloc(worker_count, sizeof(int));
  q->requests = (MPI_Request *) malloc(worker_count * sizeof(MPI_Request));

  for (i = 0; i < worker_count; ++i) {
    q->bufs[i] = q->busy_bufs[i] = NULL;
  }
}

void send_queue_add(send_queue_t *q, int dest, particle_t p) {
  if (q->count[dest] == q->capacity[dest]) {
    q->capacity[dest] += q->buf_size;
    q->bufs[dest] = (particle_t *) realloc(q->bufs[dest], q->capacity[dest] * sizeof(particle_t));
  }

  q->bufs[dest][q->count[dest]++] = p;
}

void send_queue_send(send_queue_t *q) {
  int i;

  for (i = 0; i < q->worker_count; ++i) {
    if (q->busy[i]) {
      int finished;

      MPI_Test(&q->requests[i], &finished, MPI_STATUS_IGNORE);
      q->busy[i] = !finished;
    }

    if (q->count[i] && !q->busy[i]) {
      int tmp_capacity;
      particle_t *tmp_buf;

      MPI_Isend(q->bufs[i], q->count[i], particle_type,
                i, PARTICLES_TAG, MPI_COMM_WORLD, &q->requests[i]);

      q->count[i] = 0;
      tmp_capacity = q->capacity[i];
      tmp_buf = q->bufs[i];
      q->capacity[i] = q->busy_capacity[i];
      q->bufs[i] = q->busy_bufs[i];
      q->busy_capacity[i] = tmp_capacity;
      q->busy_bufs[i] = tmp_buf;
    }
  }
}

void send_queue_destroy(send_queue_t *q) {
  int i;
  
  for (i = 0; i < q->worker_count; ++i) {
    free(q->bufs[i]);
    free(q->busy_bufs[i]);
  }

  free(q->count);
  free(q->capacity);
  free(q->bufs);

  free(q->busy_capacity);
  free(q->busy_bufs);

  free(q->busy);
  free(q->requests);
}
