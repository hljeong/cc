#include "lc.h"

Context ctx = {};

int main(int argc, char **argv) {
  if (argc < 2) errorf("usage: %s <expression> [nf=(nf)|whnf] [max-steps=10]", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);

  bool whnf = false;
  if (argc >= 3) whnf = !strcmp(argv[2], "whnf");

  int max_steps = 10;
  if (argc >= 4) max_steps = atoi(argv[3]);

  debugf("src: %s\n", ctx.lexer.loc = ctx.src);

  debug_token_stream(ctx.parser.tok = lex());

  Node *ast = parse();
  debug_ast(ast);
  debug_unparse(ast);

  Node *nxt;
  int steps = 0;
  while (ast != (nxt = step(ast, whnf))) {
    debugf("%d: ", ++steps);
    debug_unparse(ast = nxt);

    if (steps >= max_steps) {
      debugf("max reductions reached, halting\n");
      break;
    }
  }

  print_unparse(ast);

  return 0;
}

// todo: shadow warning
// todo: map node to src
