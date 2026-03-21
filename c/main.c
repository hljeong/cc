#include "cc.h"

#include <string.h>

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) errorf("usage: %s <expression>", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);

  debugf("src: %s\n", ctx.lexer.loc = ctx.src);

  debug_token_stream(ctx.parser.tok = lex());

  debug_ast(ctx.codegen.node = parse());

  codegen();

  return 0;
}
