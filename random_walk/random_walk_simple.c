#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <omp.h>

#include "random_walk.h"

int main(int argc, char **argv) {
  int a, b, x;
  int num_particles;
  double p;
  int b_count, total_time;
  struct drand48_data seed;
  int i;
  struct timespec begin_time, end_time;
  double elapsed_time;
  FILE *stats;

  if (argc != 6) {
    printf("usage: %s a b x N p", argv[0]);
    return EXIT_FAILURE;
  }

  a = atoi(argv[1]);
  b = atoi(argv[2]);
  x = atoi(argv[3]);
  num_particles = atoi(argv[4]);
  p = atof(argv[5]);

  srand48_r(time(NULL), &seed);

  b_count = total_time = 0;
  clock_gettime(CLOCK_MONOTONIC, &begin_time);
  for (i = 0; i < num_particles; ++i) {
    walk_result_t result = simulate_walk(a, b, x, p, &seed);
    b_count += result.destination;
    total_time += result.duration;
  }
  clock_gettime(CLOCK_MONOTONIC, &end_time);
  elapsed_time = end_time.tv_sec - begin_time.tv_sec
              + (end_time.tv_nsec - begin_time.tv_nsec) * 1e-9,

  stats = fopen("stats.txt", "w");
  fprintf(stats, "%.2f %.1f %f %d %d %d %d %f\n",
          (double) b_count / num_particles,
          (double) total_time / num_particles,
          elapsed_time,
          a, b, x, num_particles, p);
  fclose(stats);

  return EXIT_SUCCESS;
}
