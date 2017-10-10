#ifndef RANDOM_WALK_H_
#define RANDOM_WALK_H_

#include <stdlib.h>

typedef struct {
  int destination; /* 0 for a and 1 for b */
  int duration;
} walk_result_t;

walk_result_t simulate_walk(int a, int b, int x, double p,
                            struct drand48_data *seedp);

#endif // RANDOM_WALK_H_
