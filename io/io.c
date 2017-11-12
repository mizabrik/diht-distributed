#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

int main(int argc, char **argv) {
  int l, a, b, population;
  int rank;
  MPI_File data;
  MPI_Datatype contiguous, view;
  int i;
  int *density;
  int seed;

  struct timespec begin_time, end_time;
  double elapsed_time;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (argc != 5) {
    if (rank == 0) {
      fprintf(stderr, "usage: %s l a b N\n", argv[0]);
    }
    return EXIT_FAILURE;
  }
  l = atoi(argv[1]);
  a = atoi(argv[2]);
  b = atoi(argv[3]);
  population = atoi(argv[4]);
  density = calloc(l * l * a * b, sizeof(int));

  if (rank == 0) {
    int comm_size;
    int i;
    int *seeds;

    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    if (comm_size != a * b) {
      fprintf(stderr, "There must be a * b = %d threads\n", a * b);
      MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
    }

    seeds = malloc(comm_size * sizeof(int));
    seed = time(NULL);
    for (i = 0; i < comm_size; ++i) {
      seeds[i] = seed + i;
    }

    MPI_Scatter(seeds, 1, MPI_INT, &seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    clock_gettime(CLOCK_MONOTONIC, &begin_time);

    free(seeds);
  } else {
    MPI_Scatter(NULL, 1, MPI_INT, &seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
  }
  srand(seed);

  for (i = 0; i < population; ++i) {
    int x, y, r;
    x = rand() % l;
    y = rand() % l;
    r = rand() % (a * b);
    density[(y * l + x) * (a * b) + r] += 1;
  }

  /* open file and set file view */
  MPI_File_open(MPI_COMM_WORLD, "data.bin", MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &data);
  MPI_File_set_size(data, 0);
  MPI_Type_contiguous(l * a * b, MPI_INT, &contiguous);
  MPI_Type_create_resized(contiguous, 0, a * l * a * b * sizeof(int), &view);
  MPI_Type_commit(&view);
  MPI_File_set_view(data, ((rank / a) * a * l + rank % a) * l * a * b * sizeof(int), MPI_INT, view,
                    "native", MPI_INFO_NULL);
  MPI_File_write_all(data, density, l * l * a * b, MPI_INT, MPI_STATUS_IGNORE);

  MPI_File_close(&data);
  free(density);

  if (rank == 0) {
    FILE *stats = fopen("stats.txt", "w");

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = end_time.tv_sec - begin_time.tv_sec
                 + (end_time.tv_nsec - begin_time.tv_nsec) * 1e-9;

    fprintf(stats, "%d %d %d %d %.2fs\n", l, a, b, population, elapsed_time);

    fclose(stats);
  }

  MPI_Finalize();

  return 0;
}
