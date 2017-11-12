#include "2d_walk.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

#include <mpi.h>

#include "send_queue.h"

MPI_Datatype particle_type;

int a, b;
int l;
int lifetime;
int population;
double p_l, p_r, p_u, p_d;
int width, height;

int rank;
int comm_size;
int region_x, region_y;
int x_min, x_max;
int y_min, y_max;

int *stats = NULL;

void simulate_walk(int is_master);

particle_t random_particle();
void move_particle(particle_t *p);
int particle_distance(particle_t p);
int particle_owner(particle_t p);

void init_particle_datatype();

int main(int argc, char **argv) {
  int seed;
  struct timespec begin_time, end_time;
  double elapsed_time;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
  init_particle_datatype();

  if (argc != 10) {
    if (rank == 0) {
      fprintf(stderr, "usage: %s l a b n N pl pr pu pd\n", argv[0]);
    }
    return EXIT_FAILURE;
  }

  l = atoi(argv[1]);
  a = atoi(argv[2]);
  b = atoi(argv[3]);
  width = l * a;
  height = l * b;
  lifetime = atoi(argv[4]);
  population = atoi(argv[5]);

  region_x = rank % a;
  x_min = region_x * l;
  x_max = (region_x + 1) * l - 1;
  region_y = rank / a;
  y_min = region_y * l;
  y_max = (region_y + 1) * l - 1;

  p_l = atof(argv[6]);
  p_r = atof(argv[7]);
  p_u = atof(argv[8]);
  p_d = atof(argv[9]);

  if (comm_size != a * b) {
    if (rank == 0) {
      fprintf(stderr, "Communicator size must be a * b = %d\n", a * b);
    }
    return EXIT_FAILURE;
  }

  if (rank == 0) {
    int i;
    int *seeds = malloc(comm_size * sizeof(int));
    stats = (int *) calloc(comm_size, sizeof(int));
    
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
 
  simulate_walk(rank == 0);

  if (rank == 0) {
    int i;
    FILE *output = fopen("stats.txt", "w");

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    elapsed_time = end_time.tv_sec - begin_time.tv_sec
                 + (end_time.tv_nsec - begin_time.tv_nsec) * 1e-9;
    fprintf(output, "%d %d %d %d %d %f %f %f %f %.2fs\n",
            l, a, b, lifetime, population, p_l, p_r, p_u, p_d, elapsed_time);

    for (i = 0; i < comm_size; ++i) {
      fprintf(output, "%d: %d\n", i, stats[i]);
    }

    fclose(output);
    free(stats);
  }

  MPI_Finalize();

  return EXIT_SUCCESS;
}

void simulate_walk(int is_master) {
  int stopped = 0;
  int particle_count = 0;
  int particle_capacity = 2 * population;
  particle_t *particles = malloc(particle_capacity * sizeof(particle_t));
  int finished_count = 0;
  send_queue_t queue;

  int stats_sent = 0;
  MPI_Request stats_request;

  /* master only */
  int finished_total = 0;

  send_queue_init(&queue, comm_size, population);

  while (particle_count < population) {
    particles[particle_count++] = random_particle();
  }

  while (!stopped) {
    int i;
    int incoming = 0;

    if (particle_count == 0) {
      if (finished_count) {
        if (stats_sent) {
          MPI_Wait(&stats_request, MPI_STATUS_IGNORE);
        }
        stats_sent = finished_count;
        MPI_Isend(&stats_sent, 1, MPI_INT,
                  0, STATS_TAG, MPI_COMM_WORLD, &stats_request);
        finished_count = 0;
      }

      if (is_master && finished_total == comm_size * population) {
        /* we're done */
        MPI_Request *stop_requests = malloc((comm_size - 1) * sizeof(MPI_Request));

        stopped = 1;
        for (i = 0; i < comm_size - 1; ++i) {
          MPI_Isend(&stopped, 1, MPI_INT,
                    i + 1, STOP_TAG, MPI_COMM_WORLD, &stop_requests[i]);
        }
        MPI_Waitall(comm_size - 1, stop_requests, MPI_STATUSES_IGNORE);

        free(stop_requests);
      }  else {
        MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
    }

    do {
      MPI_Status s;
      MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &incoming, &s);

      if (incoming) {
        int incoming_count;
        int incoming_stats;
        MPI_Request pong_request;

        switch (s.MPI_TAG) {
          case PARTICLES_TAG:
            MPI_Get_count(&s, particle_type, &incoming_count);

            while (particle_count + incoming_count > particle_capacity) {
              particle_capacity *= 2;
              particles = realloc(particles, particle_capacity * sizeof(particle_t));
            }
            MPI_Recv(particles + particle_count, incoming_count, particle_type,
                     s.MPI_SOURCE, PARTICLES_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            particle_count += incoming_count;

            /* used to wake up thread, if it only has particles waiting for send */
            MPI_Isend(&rank, 0, MPI_INT,
                      s.MPI_SOURCE, PONG_TAG, MPI_COMM_WORLD, &pong_request);
            MPI_Request_free(&pong_request);
            break;
          case PONG_TAG:
            MPI_Recv(&incoming_stats, 0, MPI_INT,
                     s.MPI_SOURCE, PONG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            break;
          case STATS_TAG:
            /* only for master */
            MPI_Recv(&incoming_stats, 1, MPI_INT,
                     s.MPI_SOURCE, STATS_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            stats[s.MPI_SOURCE] += incoming_stats;
            finished_total += incoming_stats;
            break;
          case STOP_TAG:
            MPI_Recv(&stopped, 1, MPI_INT,
                     s.MPI_SOURCE, STOP_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            break;
        }
      }
    } while (incoming);

    for (i = 0; i < particle_count; ++i) {
      /* we remove finished particles during the next step to move them */
      if (particles[i].time == lifetime) {
        ++finished_count;
        particles[i--] = particles[--particle_count];
      } else {
        int distance;

        move_particle(&particles[i]);

        distance = particle_distance(particles[i]);
        if (distance > STIKINESS
            || (particles[i].time == lifetime && distance > 0)) {
          send_queue_add(&queue, particle_owner(particles[i]), particles[i]);
          particles[i--] = particles[--particle_count];
        }
      }
    }
    send_queue_send(&queue);
  }

  /* free requests */
  if (stats_sent) {
    MPI_Wait(&stats_request, MPI_STATUS_IGNORE);
  }
  send_queue_send(&queue); 

  send_queue_destroy(&queue);
  free(particles);
}

particle_t random_particle() {
  particle_t p;

  p.x = x_min + rand() % l;
  p.y = y_min + rand() % l;
  p.time = 0;

  return p;
}

void move_particle(particle_t *p){
  double x = (double) rand() / RAND_MAX;

  if (x < p_l) {
    p->x = (width + p->x - 1) % width;
  } else if (x < p_l + p_r) {
    p->x = (p->x + 1) % width;
  } else if (x < p_l + p_r + p_u) {
    p->y = (p->y + 1) % height;
  } else {
    p->y = (height + p->y - 1) % height;
  }

  ++p->time;
}

int particle_distance(particle_t p) {
  int d_x, d_y;

  if (p.x < x_min) {
    d_x = MIN(x_min - p.x, l * a + p.x - x_max);
  } else if (p.x > x_max) {
    d_x = MIN(p.x - x_max, l * a + x_min - p.x);
  } else {
    d_x = 0;
  }
  if (p.y < y_min) {
    d_y = MIN(y_min - p.y, l * b + p.y - y_max);
  } else if (p.y > y_max) {
    d_y = MIN(p.y - y_max, l * b + y_min - p.y);
  } else {
    d_y = 0;
  }

  return MAX(d_x, d_y);
}

int particle_owner(particle_t p) {
  return (p.x / l) + (p.y / l) * a;
}

void init_particle_datatype() {
  int lens[3] = { 1, 1, 1 };
  MPI_Aint displacements[3] = {
    offsetof(particle_t, x), offsetof(particle_t, y), offsetof(particle_t, time)
  };
  MPI_Datatype types[3] = { MPI_INT, MPI_INT, MPI_INT };

  MPI_Type_create_struct(3, lens, displacements, types, &particle_type);
  MPI_Type_commit(&particle_type);
}
