#include "rc.h"

#include <stdio.h>
#include <stdlib.h>

Context ctx;

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s <pattern> <text>\n",
            argv[0]);
    exit(1);
  }

  const char *pattern = argv[1];
  const char *text = argv[2];

  ctx.src = pattern;
  lex();
  parse();
}
