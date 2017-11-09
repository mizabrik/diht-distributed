#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <pthread.h>

typedef struct {
  int (* func) (void *);
  void *arg;

  int id;
  int finished;
  int result;
} task_t;

typedef struct {
  int destroyed;
  int thread_count;
  int started_threads;

  pthread_t *threads;
  pthread_cond_t *cvs;
  task_t **tasks;
  int *has_task;

  pthread_mutex_t lock;
} thread_pool_t;

void thread_pool_init(thread_pool_t *pool, int thread_count);

int thread_pool_try_submit(thread_pool_t *pool, task_t *task);

int thread_pool_wait(thread_pool_t *pool, task_t *task);

/* must not be called concurrently with thread_pool_wait */
void thread_pool_destroy(thread_pool_t *pool);

#endif // THREAD_POOL_H_
