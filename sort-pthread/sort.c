#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "thread_pool.h"

int comparator(const void *x_ptr, const void *y_ptr);

void dump_array(FILE *f, const int *array, int n);

size_t find(void *value, void *array, size_t n, size_t size,
            int (*compar)(const void *, const void *));

void parallelsort(void *array, size_t n, size_t size,
                  int (*compar)(const void *, const void *),
                  size_t chunk_size, thread_pool_t *pool);

typedef struct {
  void *array;
  void *buffer;
  size_t n;
  size_t size;
  int (*compar)(const void *, const void *);
  size_t chunk_size;
  thread_pool_t *pool;
} mergesort_args_t;
int mergesort(void *arg);

typedef struct {
  void *larray;
  void *rarray;
  void *array;
  size_t ln;
  size_t rn;
  size_t size;
  int (*compar)(const void *, const void *);
} merge_args_t;
int merge(void *arg);

int main(int argc, char **argv) {
  int n;
  int chunk_size;
  int num_threads;
  struct timespec begin_time, end_time;
  double elapsed_time;
  FILE *stats, *data;
  int *array;
  int i;
  thread_pool_t pool;

  if (argc != 4) {
    fprintf(stderr, "usage: %s n m P", argv[0]);
    return EXIT_FAILURE;
  }

  n = atoi(argv[1]);
  chunk_size = atoi(argv[2]);
  num_threads = atoi(argv[3]);

  srand(time(NULL));
  data = fopen("data.txt", "w");

  array = malloc(n * sizeof(int));
  for (i = 0; i < n; ++i) {
    array[i] = rand();
  }
  dump_array(data, array, n);

  if (num_threads > 0) {
    thread_pool_init(&pool, num_threads - 1); /* main thread gives -1 */
  }

  clock_gettime(CLOCK_MONOTONIC, &begin_time);

  if (num_threads > 0) {
    parallelsort(array, n, sizeof(int), comparator, chunk_size, &pool);
  } else {
    qsort(array, n, sizeof(int), comparator);
  }

  clock_gettime(CLOCK_MONOTONIC, &end_time);
  elapsed_time = end_time.tv_sec - begin_time.tv_sec
               + (end_time.tv_nsec - begin_time.tv_nsec) * 1e-9;

  if (num_threads > 0) {
    thread_pool_destroy(&pool);
  }

  dump_array(data, array, n);
  fclose(data);

  stats = fopen("stats.txt", "w");
  fprintf(stats, "%f %d %d %d\n", elapsed_time,
          n, chunk_size, num_threads);
  fclose(stats);
  
  free(array);

  return EXIT_SUCCESS;
}

int comparator(const void *x_ptr, const void *y_ptr) {
  int x, y;
  x = *(const int *)x_ptr;
  y = *(const int *)y_ptr;

  if (x < y) {
    return -1;
  } else if (x > y) {
    return 1;
  } else {
    return 0;
  }
}

void dump_array(FILE *f, const int *array, int n) {
  int i;

  if (n <= 0)
    return;

  fprintf(f, "%d", array[0]);
  for (i = 1; i < n; ++i)
    fprintf(f, " %d", array[i]);
  fputc('\n', f);
}

void parallelsort(void *array, size_t n, size_t size,
                  int (*compar)(const void *, const void *),
                  size_t chunk_size, thread_pool_t *pool) {
  int in_buf;
  mergesort_args_t args;

  args.array = array;
  args.buffer = malloc(n * size);
  args.n = n;
  args.size = size;
  args.compar = compar;
  args.chunk_size = chunk_size;
  args.pool = pool;

  in_buf = mergesort(&args);
  if (in_buf) {
    memcpy(array, args.buffer, n * size);
  }

  free(args.buffer);
}

int mergesort(void *arg) {
  mergesort_args_t *args = (mergesort_args_t *) arg;
  size_t mid;
  size_t lmid, rmid;
  char *data, *buf;
  int left_in_buf, right_in_buf;
  mergesort_args_t largs_sort, rargs_sort;
  merge_args_t largs_merge, rargs_merge;
  task_t task_sort, task_merge;
  
  if (args->n <= args->chunk_size) {
    qsort(args->array, args->n, args->size, args->compar);
    return 0;
  }

  data = (char *) args->array;
  buf = (char *) args->buffer;
  mid = args->n / 2;

  largs_sort = rargs_sort = *args;
  largs_sort.n = mid;
  rargs_sort.array = data + args->size * mid;
  rargs_sort.buffer = buf + args->size * mid;
  rargs_sort.n -= mid;

  task_sort.func = mergesort;
  task_sort.arg = &largs_sort;

  if (thread_pool_try_submit(args->pool, &task_sort)) {
    right_in_buf = mergesort(&rargs_sort);
    left_in_buf = thread_pool_wait(args->pool, &task_sort);
  } else {
    left_in_buf = mergesort(&largs_sort);
    right_in_buf = mergesort(&rargs_sort);
  }

  /* we move right part only once, and pretend it's in buf */
  if (!right_in_buf) {
    char *tmp;
    tmp = data;
    data = buf;
    buf = tmp;
  }

  /* because merge is parallel, everything must be in same array */
  if (left_in_buf != right_in_buf) {
    memcpy(buf, data, args->size * mid);
  }

  lmid = mid / 2;
  rmid = mid + find(buf + args->size * lmid, buf + args->size * mid,
                    args->n - mid, args->size, args->compar);

  largs_merge.size = rargs_merge.size = args->size;
  largs_merge.compar = rargs_merge.compar = args->compar;

  largs_merge.larray = buf;
  largs_merge.rarray = buf + args->size * mid;
  largs_merge.array = data;
  largs_merge.ln = lmid;
  largs_merge.rn = rmid - mid;

  rargs_merge.larray = buf + args->size * lmid;
  rargs_merge.rarray = buf + args->size * rmid;
  rargs_merge.array = data + args->size * (lmid + (rmid - mid));
  rargs_merge.ln = mid - lmid;
  rargs_merge.rn = args->n - rmid;

  task_merge.func = merge;
  task_merge.arg = &largs_merge;

  if (thread_pool_try_submit(args->pool, &task_merge)) {
    merge(&rargs_merge);
    thread_pool_wait(args->pool, &task_merge);
  } else {
    merge(&largs_merge);
    merge(&rargs_merge);
  }

  return !right_in_buf;
}

/* возвращает минимальное i т. ч. array[i] >= value */
size_t find(void *value, void *array, size_t n, size_t size,
            int (*compar)(const void *, const void *)) {
  size_t mid, l, r;
  char *data;
 
  data = (char *) array;

  l = 0;
  r = n - 1;

  if (compar(data + size * r, value) < 0)
    return n;

  while (l + 1 < r) {
    mid = l + (r - l) / 2;
   
    if (compar(data + size * mid, value) < 0) {
      l = mid + 1;
    } else {
      r = mid;
    }
  }

  return compar(data + size * l, value) < 0 ? r : l;
}

int merge(void *arg) {
  merge_args_t *args;
  char *ldata, *rdata, *data;
  size_t ln, rn, size;
  size_t i, j, out;
  i = j = 0;
  out = 0;

  args = (merge_args_t *) arg;
  ldata = (char *) args->larray;
  rdata = (char *) args->rarray;
  data = (char *) args->array;
  ln = args->ln;
  rn = args->rn;
  size = args->size;

  while (i < ln && j < rn) {
    if (args->compar(ldata + size * i, rdata + size * j) <= 0) {
      memcpy(data + size * out, ldata + size * i, size);
      ++i;
    } else {
      memcpy(data + size * out, rdata + size * j, size);
      ++j;
    }
    ++out;
  }

  while (i < ln) {
    memcpy(data + size * out, ldata + size * i, size);
    ++i;
    ++out;
  }

  while (j < rn) {
    memcpy(data + size * out, rdata + size * j, size);
    ++j;
    ++out;
  }

  return 0;
}
