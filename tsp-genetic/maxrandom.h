#ifndef MAXRANDOM_H_
#define MAXRANDOM_H_

#include <stddef.h>

void maxrandom_init(unsigned int seed, size_t pack_size, size_t max_pack_count);
void maxrandom_destroy();

typedef struct {
  int *pack;
  size_t used;
} maxrandom_data_t;

void maxrandom_data_init(maxrandom_data_t *data);
void maxrandom_data_destroy(maxrandom_data_t *data);

int maxrandom(maxrandom_data_t *data);

#endif /* MAXRANDOM_H_ */
