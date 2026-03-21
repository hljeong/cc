#include "cc.h"

#include <string.h>

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) errorf("usage: %s <expression>\n", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);

  debugf(ctx.lexer.loc = ctx.src); debugf("\n");

  debug_token_stream(ctx.parser.tok = lex());

  debug_ast(ctx.codegen.node = parse());

  codegen();

  return 0;
}
