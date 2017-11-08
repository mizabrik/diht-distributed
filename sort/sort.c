#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <omp.h>

int comparator(const void *x_ptr, const void *y_ptr);

void dump_array(FILE *f, const int *array, int n);

size_t find(void *value, void *array, size_t n, size_t size,
            int (*compar)(const void *, const void *));

void merge(void *larray, void *rarray, void *array,
           size_t ln, size_t rn, size_t size,
           int (*compar)(const void *, const void *));

void parallelsort(void *array, size_t n, size_t size,
                    int (*compar)(const void *, const void *),
                    size_t chunk_size);

int mergesort(void *array, void *buffer, size_t n, size_t size,
              int (*compar)(const void *, const void *),
              size_t chunk_size);

int main(int argc, char **argv) {
  int n;
  int chunk_size;
  int num_threads;
  double begin_time, end_time;
  FILE *stats, *data;
  int *array;
  int i;

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

  begin_time = omp_get_wtime();

  if (num_threads > 0) {
    omp_set_num_threads(num_threads);
    parallelsort(array, n, sizeof(int), comparator, chunk_size);
  } else {
    qsort(array, n, sizeof(int), comparator);
  }

  end_time = omp_get_wtime();

  dump_array(data, array, n);
  fclose(data);

  stats = fopen("stats.txt", "w");
  fprintf(stats, "%f %d %d %d\n", end_time - begin_time,
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
                  size_t chunk_size) {
  void *buf;
  int in_buf;

  buf = malloc(n * size);

  in_buf = mergesort(array, buf, n, size, compar, chunk_size);
  if (in_buf) {
    memcpy(array, buf, n * size);
  }

  free(buf);
}

int mergesort(void *array, void *buffer, size_t n, size_t size,
              int (*compar)(const void *, const void *),
              size_t chunk_size) {
  size_t mid;
  size_t lmid, rmid;
  char *data, *buf;
  int left_in_buf, right_in_buf;
  
  if (n <= chunk_size) {
    qsort(array, n, size, compar);
    return 0;
  }

  data = (char *) array;
  buf = (char *) buffer;
  mid = n / 2;

  #pragma omp parallel sections
  {
    #pragma omp section
    left_in_buf = mergesort(data, buf, mid, size, compar, chunk_size);

    #pragma omp section
    right_in_buf = mergesort(data + size * mid, buf + size * mid,
                             n - mid, size, compar, chunk_size);
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
    memcpy(buf, data, size * mid);
  }

  lmid = mid / 2;
  rmid = mid + find(buf + size * lmid, buf + size * mid,
                    n - mid, size, compar);

  #pragma omp parallel sections
  {
    #pragma omp section
    merge(buf, buf + size * mid, data, lmid, rmid - mid, size, compar);

    #pragma omp section
    merge(buf + size * lmid, buf + size * rmid,
          data + size * (lmid + (rmid - mid)),
          mid - lmid, n - rmid, size, compar);
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

void merge(void *larray, void *rarray, void *array,
           size_t ln, size_t rn, size_t size,
           int (*compar)(const void *, const void *)) {
  char *ldata, *rdata, *data;
  size_t i, j, out;
  i = j = 0;
  out = 0;

  ldata = (char *) larray;
  rdata = (char *) rarray;
  data = (char *) array;

  while (i < ln && j < rn) {
    if (compar(ldata + size * i, rdata + size * j) <= 0) {
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
}
