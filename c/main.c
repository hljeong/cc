#include "cc.h"

#include <string.h>

bool enable_debug = true;

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) error_f("usage: %s <expression>", argv[0]);

  ctx.src = sv_create(argv[1], strlen(argv[1]));
  if (enable_debug) debug_f("src: %s", ctx.src);

  lex();
  if (enable_debug) debug_f("%{token_stream}", ctx.toks);

  parse();
  if (enable_debug) debug_f("%{ast}", ctx.ast);

  analyze();
  if (enable_debug) debug_f("%{ast}", ctx.ast);

  codegen();

  return 0;
}
