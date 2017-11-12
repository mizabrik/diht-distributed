#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "graph.h"

int main(int argc, char **argv) {
  FILE *stats;
  graph_t *graph;
  int *counts;
  int *route;
  int weight, real_weight;
  int i;

  if (argc != 2) {
    fprintf(stderr, "usage: %s GRAPH\n", argv[0]);
    return EXIT_FAILURE;
  }

  graph = graph_read_file(argv[1]);
  route = malloc(graph->n * sizeof(int));
  counts = calloc(graph->n, sizeof(int));

  stats = fopen("stats.txt", "r");
  fscanf(stats, "%*d %*d %*d %*fs %d", &weight);
  for (i = 0; i < graph->n; ++i) {
    fscanf(stats, "%d", route + i);
    counts[route[i]] += 1;
  }

  for (i = 0; i < graph->n; ++i) {
    if (counts[i] != 1) {
      puts("FAIL");
      return EXIT_FAILURE;
    }
  }

  real_weight = graph_weight(graph, route[graph->n - 1], route[0]);
  for (i = 1; i < graph->n; ++i) {
    real_weight += graph_weight(graph, route[i - 1], route[i]);
  }

  if (weight != real_weight) {
    puts("FAIL");
    return EXIT_FAILURE;
  }

  puts("OK");

  graph_destroy(graph);

  return EXIT_SUCCESS;
}
