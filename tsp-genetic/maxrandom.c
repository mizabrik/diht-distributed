#include "maxrandom.h"

#include <stdlib.h>
#include <pthread.h>

static size_t maxrandom_pack_size;
static size_t maxrandom_max_pack_count;

static int **maxrandom_packs;
static int maxrandom_destroyed = 0;
static unsigned int maxrandom_state;
static size_t maxrandom_pack_count = 0;

static pthread_t maxrandom_thread;
static pthread_mutex_t maxrandom_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t maxrandom_cond_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t maxrandom_cond_empty = PTHREAD_COND_INITIALIZER;

void * maxrandom_generator(void *arg) {
  pthread_mutex_lock(&maxrandom_lock);
  for (;;) {
    int *pack;
    size_t i;

    while (!(maxrandom_destroyed
             || maxrandom_pack_count < maxrandom_max_pack_count)) {
      pthread_cond_wait(&maxrandom_cond_full, &maxrandom_lock);
    }

    if (maxrandom_destroyed)
      break;

    pthread_mutex_unlock(&maxrandom_lock);
    pack = (int *) malloc(maxrandom_pack_size * sizeof(int));
    for (i = 0; i < maxrandom_pack_size; ++i) {
      pack[i] = rand_r(&maxrandom_state);
    }
    pthread_mutex_lock(&maxrandom_lock);

    if (!maxrandom_pack_count) {
      pthread_cond_signal(&maxrandom_cond_empty);
    }
    maxrandom_packs[maxrandom_pack_count++] = pack;
  }
  pthread_mutex_unlock(&maxrandom_lock);

  return NULL;
}

void maxrandom_init(unsigned int seed, size_t pack_size, size_t max_pack_count) {
  maxrandom_pack_size = pack_size;
  maxrandom_max_pack_count = max_pack_count;
  maxrandom_state = seed;
  maxrandom_packs = (int **) malloc(maxrandom_max_pack_count * sizeof(int *));

  pthread_create(&maxrandom_thread, NULL, maxrandom_generator, NULL);
}

void maxrandom_destroy() {
  pthread_mutex_lock(&maxrandom_lock);
  maxrandom_destroyed = 1;
  while (maxrandom_pack_count) {
    free(maxrandom_packs[--maxrandom_pack_count]);
  }
  free(maxrandom_packs);
  pthread_mutex_unlock(&maxrandom_lock);

  pthread_cond_signal(&maxrandom_cond_full);
  pthread_join(maxrandom_thread, NULL);

  pthread_mutex_destroy(&maxrandom_lock);
  pthread_cond_destroy(&maxrandom_cond_full);
  pthread_cond_destroy(&maxrandom_cond_empty);
}

void maxrandom_data_init(maxrandom_data_t *data) {
  data->used = maxrandom_pack_size;
  data->pack = NULL;
}

void maxrandom_data_destroy(maxrandom_data_t *data) {
  free(data->pack);
}

int maxrandom(maxrandom_data_t *data) {
  if (data->used == maxrandom_pack_size) {
    data->used = 0;
    free(data->pack);

    pthread_mutex_lock(&maxrandom_lock);
    while (!maxrandom_pack_count) {
      pthread_cond_wait(&maxrandom_cond_empty, &maxrandom_lock);
    }
    data->pack = maxrandom_packs[--maxrandom_pack_count];
    pthread_mutex_unlock(&maxrandom_lock);

    pthread_cond_signal(&maxrandom_cond_full);
  }

  return data->pack[data->used++];
}
