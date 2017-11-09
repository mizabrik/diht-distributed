#include "thread_pool.h"

#include <stdlib.h>

#include <pthread.h>

void * thread_pool_worker(void *arg) {
  int id;
  thread_pool_t *pool; 
  
  pool = (thread_pool_t *) arg;
  pthread_mutex_lock(&pool->lock);
  id = pool->started_threads++;
  for (;;) {
    while (!(pool->destroyed || pool->has_task[id])) {
      pthread_cond_wait(&pool->cvs[id], &pool->lock);
    }

    if (pool->has_task[id]) {
      int result;
      task_t *task;

      task = pool->tasks[id];
      pthread_mutex_unlock(&pool->lock);
      result = task->func(task->arg);
      pthread_mutex_lock(&pool->lock);

      task->result = result;
      task->finished = 1;
      pool->has_task[id] = 0;
      pthread_cond_signal(&pool->cvs[id]);
    } else {
      break; /* pool is destroyed */
    }
  }

  pthread_mutex_unlock(&pool->lock);

  return NULL;
}

void thread_pool_init(thread_pool_t *pool, int thread_count) {
  int i;

  pool->destroyed = 0;
  pool->thread_count = thread_count;
  pool->started_threads = 0;

  pool->threads = (pthread_t *) malloc(thread_count * sizeof(pthread_t));
  pool->cvs = (pthread_cond_t *) malloc(thread_count * sizeof(pthread_cond_t));
  pool->tasks = (task_t **) malloc(thread_count * sizeof(task_t *));
  pool->has_task = (int *) calloc(thread_count, sizeof(int));

  pthread_mutex_init(&pool->lock, NULL);

  for (i = 0; i < thread_count; ++i) {
    pthread_cond_init(&pool->cvs[i], NULL);
    pthread_create(&pool->threads[i], NULL, thread_pool_worker, pool);
  }
}

int thread_pool_try_submit(thread_pool_t *pool, task_t *task) {
  int i;

  pthread_mutex_lock(&pool->lock);

  if (pool->destroyed) {
    return 0;
  }

  for (i = 0; i < pool->thread_count; ++i) {
    if (!pool->has_task[i]) {
      task->id = i;
      task->finished = 0;
      pool->tasks[i] = task;
      pool->has_task[i] = 1;

      pthread_mutex_unlock(&pool->lock);
      pthread_cond_signal(&pool->cvs[i]);

      return 1;
    }
  }

  pthread_mutex_unlock(&pool->lock);

  return 0;
}

int thread_pool_wait(thread_pool_t *pool, task_t *task) {
  pthread_mutex_lock(&pool->lock);

  while (!task->finished) {
    pthread_cond_wait(&pool->cvs[task->id], &pool->lock);
  }

  pthread_mutex_unlock(&pool->lock);

  return task->result;
}

void thread_pool_destroy(thread_pool_t *pool) {
  int i;

  pthread_mutex_lock(&pool->lock);
  pool->destroyed = 1;
  pthread_mutex_unlock(&pool->lock);

  /* real thread order might be random */
  for (i = 0; i < pool->thread_count; ++i) {
    pthread_cond_signal(&pool->cvs[i]);
  }

  for (i = 0; i< pool->thread_count; ++i) {
    pthread_join(pool->threads[i], NULL);
  }

  for (i = 0; i< pool->thread_count; ++i) {
    pthread_cond_destroy(&pool->cvs[i]);
  }


  free(pool->threads);
  free(pool->cvs);
  free(pool->tasks);
  free(pool->has_task);

  pthread_mutex_destroy(&pool->lock);
}
