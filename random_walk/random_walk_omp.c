#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <omp.h>

#include "random_walk.h"

int main(int argc, char **argv) {
  int a, b, x;
  int num_particles;
  double p;
  int num_threads;
  int b_count, total_time;
  unsigned int seed_base;
  int i;
  double begin_time, end_time;
  FILE *stats;

  if (argc != 7) {
    fprintf(stderr, "usage: %s a b x N p P", argv[0]);
    return EXIT_FAILURE;
  }

  a = atoi(argv[1]);
  b = atoi(argv[2]);
  x = atoi(argv[3]);
  num_particles = atoi(argv[4]);
  p = atof(argv[5]);
  num_threads = atoi(argv[6]);

  omp_set_num_threads(num_threads);

  seed_base = time(NULL);

  b_count = total_time = 0;
  begin_time = omp_get_wtime();

  #pragma omp parallel
  {
    struct drand48_data seed;
    srand48_r(seed_base + omp_get_thread_num(), &seed);

    #pragma omp for reduction(+ : b_count, total_time)
    for (i = 0; i < num_particles; ++i) {
      walk_result_t result = simulate_walk(a, b, x, p, &seed);
      b_count += result.destination;
      total_time += result.duration;
    }
  }
  end_time = omp_get_wtime();

  stats = fopen("stats.txt", "w");
  fprintf(stats, "%.2f %.1f %f %d %d %d %d %f %d\n",
          (double) b_count / num_particles,
          (double) total_time / num_particles,
          end_time - begin_time,
          a, b, x, num_particles, p, num_threads);
  fclose(stats);

  return EXIT_SUCCESS;
}
