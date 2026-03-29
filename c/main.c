#include "cc.h"

bool enable_debug = true;

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) errorf("usage: %s <expression>", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);

  ctx.lexer.loc = ctx.src;
  if (enable_debug) debugf("src: %s\n", ctx.lexer.loc);

  ctx.parser.tok = lex();
  if (enable_debug) debug(str_token_stream(ctx.parser.tok));

  ctx.ast = parse();
  if (enable_debug) debug(str_ast(ctx.ast));

  analyze();
  if (enable_debug) debug(str_ast(ctx.ast));

  codegen();

  return 0;
}
