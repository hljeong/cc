#include "lc.h"

Context ctx = {};

// lambda calculus interpreter.
// usage: lc <expr> [whnf|nf|benf] [max-steps]
//
// modes:
//   whnf - weak head normal form
//   nf   - beta-normal form
//   benf - beta-eta-normal form
//
// reduces the expression and prints the reduced form to stdout
// debug output (lexer output, ast, source spans, reduction trace) goes to stderr.
//
// example:
//   $ lc '(\x.\y.x) a b' 2>/dev/null
//   a                    // stdout (result)
int main(int argc, char **argv) {
  if (argc < 2) errorf("usage: %s <expr> [whnf|nf|benf (default=nf)] [max-steps=10]", argv[0]);

  ctx.src = argv[1];
  ctx.src_len = strlen(ctx.src);
  const NormalForm nf = (argc >= 3) ? !strcmp(argv[2], "whnf") ? NormalForm_WEAK_HEAD
                                    : !strcmp(argv[2], "benf") ? NormalForm_BETA_ETA
                                                               : NormalForm_BETA
                                    : NormalForm_BETA;
  const int max_steps = (argc >= 4) ? atoi(argv[3]) : 10;

  debugf("src: %s\n", ctx.lexer.loc = ctx.src);

  debug_token_stream(ctx.parser.tok = lex());

  Node *ast = parse();
  debug_ast(ast);
  debug_unparse(ast);

  for (int steps = 1; steps <= max_steps; steps++) {
    Node *nxt = step(ast, nf);
    if (ast == nxt) break;
    debugf("%d: ", steps);
    debug_unparse(ast = nxt);
  }

  print_unparse(ast);

  return 0;
}

// todo: colors
// todo: colored src + rainbow parens
