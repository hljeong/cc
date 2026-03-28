#include "cc.h"

bool debug = true;

Context ctx = {};

int main(int argc, char **argv) {
  if (argc != 2) errorf("usage: %s <expression>", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);

  ctx.lexer.loc = ctx.src;
  if (debug) debugf("src: %s\n", ctx.lexer.loc);

  ctx.parser.tok = lex();
  if (debug) token_stream(DEBUG, ctx.parser.tok);

  ctx.ast = parse();
  if (debug) ast(DEBUG, ctx.ast);

  analyze();
  if (debug) ast(DEBUG, ctx.ast);

  codegen();

  return 0;
}
