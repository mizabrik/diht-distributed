#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  int l, a, b, population;
  int i;
  int *counts;
  FILE *f;
  int *buffer;
  int x, y, u, v, r;
  int ok = 1;

  if (argc != 5) {
    fprintf(stderr, "usage: %s l a b N\n", argv[0]);
    return EXIT_FAILURE;
  }
  l = atoi(argv[1]);
  a = atoi(argv[2]);
  b = atoi(argv[3]);
  population = atoi(argv[4]);
  counts = calloc(a * b, sizeof(int));

  buffer = malloc(l*l*a*a*b*b * sizeof(int));
  f = fopen("data.bin", "rb");

  fread(buffer, sizeof(int), l*l*a*a*b*b, f);

  for (x = 0; x < a; ++x)
    for (y = 0; y < b; ++y)
      for (u = 0; u < l; ++u)
        for (v = 0; v < l; ++v)
          for (r = 0; r < a * b; ++r)
            counts[a * y + x] += buffer[(y * l + v) * a * l * a * b + (x * l + u) * a * b + r];

  for (i = 0; i < a * b; ++i)
    ok &= counts[i] == population;
  puts(ok ? "OK" : "FAIL");

  fclose(f);
  free(buffer);
  free(counts);

  return EXIT_SUCCESS;
}
