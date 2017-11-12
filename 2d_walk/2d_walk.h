#ifndef TWOD_WALK_H_
#define TWOD_WALK_H_

#include <mpi.h>

#define MIN(x, y) ( (x) < (y) ? (x) : (y) )
#define MAX(x, y) ( (x) > (y) ? (x) : (y) )

/* we compute movement of particle for some time after it leaves our area */
#define STIKINESS 5

enum TAGS {
  SEED_TAG,
  PARTICLES_TAG,
  PONG_TAG,
  STATS_TAG,
  STOP_TAG
};

typedef struct {
  int x;
  int y;
  int time;
} particle_t;
extern MPI_Datatype particle_type;

#endif /* TWOD_WALK_H_ */
