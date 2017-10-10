#include "random_walk.h"

#include <stdlib.h>

walk_result_t simulate_walk(int a, int b, int x, double p, 
                            struct drand48_data *seedp) {
  walk_result_t result;
  double rnd;

  result.duration = 0;
  while (x != a && x != b) {
    drand48_r(seedp, &rnd);
    result.duration += 1;
    x += 2 * (rnd < p) - 1;
  }
  result.destination = x == b;

  return result;
}
