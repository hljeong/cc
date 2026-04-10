#include "lc.h"

#include <stdio.h>

Context ctx = {};

// todo: argparse
// lambda calculus interpreter.
// usage: lc <path> [mode=whnf|nf|benf] [max-steps] [syntax=std|ext]
//
// modes:
//   whnf - weak head normal form
//   nf   - beta-normal form
//   benf - beta-eta-normal form
//
// syntax:
//   std - standard
//   ext - extended
//
// <path> can be a file path or - for stdin
//
// reduces the expression and prints the reduced form to stdout
// debug output (lexer output, ast, source spans, reduction trace) goes to stderr.
//
// example:
//   $ lc file.lc 2>/dev/null
int main(int argc, char **argv) {
  if (argc < 2)
    error("usage: %s <path> [mode=whnf|nf|benf (default=nf)] [max-steps=10000] [syntax=std|ext (default=ext)]", argv[0]);

  const char *path = argv[1];
  FILE *file = !strcmp(path, "-") ? stdin : fopen(path, "r");
  if (!file)
    error("failed to open file: %s", path);

  str_builder sb = sb_create(256);
  {
    char c;
    while ((c = fgetc(file)) != EOF)
      sb_append(&sb, "%c", c);
  }

  if (file != stdin && fclose(file))
    error("failed to close file: %s", path);

  ctx.src = sb.buf;
  ctx.src_len = strlen(ctx.src);
  const NormalForm nf = (argc >= 3) ? !strcmp(argv[2], "whnf") ? NormalForm_WEAK_HEAD
                                    : !strcmp(argv[2], "benf") ? NormalForm_BETA_ETA
                                                               : NormalForm_BETA
                                    : NormalForm_BETA;
  const int max_steps = (argc >= 4) ? atoi(argv[3]) : 10000;
  const bool ext = (argc >= 5) ? !strcmp(argv[4], "ext") : true;

  register_formatters();

  debug("src: %s\n", ctx.lexer.loc = ctx.src);

  debug("{token_stream}\n", ctx.parser.tok = lex());

  Node *ast = parse();
  debug("{ast}", ast);
  debug("{lambda}\n", ast, ext);

  for (int steps = 1; steps <= max_steps; steps++) {
    Node *nxt = step(ast, nf);
    if (ast == nxt) break;
    debug("%d: ", steps);
    debug("{lambda}\n", ast, ext);
    ast = nxt;
  }

  print("{lambda}\n", ast, ext);

  return 0;
}

// todo: colors
// todo: colored src + rainbow parens

#define CFMT_IMPL
#include "cfmt.h"
