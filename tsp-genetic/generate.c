#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "graph.h"

int main(int argc, char **argv) {
  graph_t *graph;

  if (argc < 2 || argc > 4) {
    fprintf(stderr, "usage: %s n [w] [seed]\n", argv[0]);
    return EXIT_FAILURE;
  }

  if (argc == 4) {
    srand(atoi(argv[3]));
  } else {
    srand(time(NULL));
  }
  graph = graph_generate(atoi(argv[1]), argc > 2 ? atoi(argv[2]) : 100);

  graph_dump(graph, stdout);

  graph_destroy(graph);

  return EXIT_SUCCESS;
}
