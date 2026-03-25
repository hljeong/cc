#include "cc.h"

#include <string.h>

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) errorf("usage: %s <expression>", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);

  ctx.lexer.loc = ctx.src;
  // debugf("src: %s\n", ctx.lexer.loc);

  ctx.parser.tok = lex();
  // debug_token_stream(ctx.parser.tok);

  ctx.codegen.node = parse();
  // debug_ast(ctx.codegen.node);

  codegen();

  return 0;
}
