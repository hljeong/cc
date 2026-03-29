#include "cc.h"

#include <string.h>

bool enable_debug = true;

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) error("usage: %s <expression>", argv[0]);

  ctx.src = sv_create(argv[1], strlen(argv[1]));
  if (enable_debug) debug("src: %s", ctx.src);

  lex();
  if (enable_debug) debug("%{token_stream}", ctx.toks);

  parse();
  if (enable_debug) debug("%{ast}", ctx.ast);

  analyze();
  if (enable_debug) debug("%{ast}", ctx.ast);

  codegen();

  return 0;
}
