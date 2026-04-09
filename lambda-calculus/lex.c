#include "lc.h"

#include <ctype.h>

static int is_ident_first(int c) {
  return isalpha(c) || (c == '_');
}

static int is_ident_rest(int c) {
  return isalnum(c) || (c == '_');
}

// helper semantics: consume...() attempts to
// match some expected pattern of characters,
// advance the cursor by the number of characters
// matched, and return that number. if it fails
// to match, 0 is returned

// consume as many characters `ch` as possible where
// `pred(ch)` returns true
static int consume_pred(int (*pred)(int)) {
  assert(!pred('\0'), "predicate accepts eof");
  const char *start = ctx.lexer.loc;
  while (pred(*ctx.lexer.loc)) ctx.lexer.loc++;
  return ctx.lexer.loc - start;
}

static int consume_ident() {
  int len = 0;
  if (!(len = consume_pred(is_ident_first))) return 0;
  return (len += consume_pred(is_ident_rest));
}

static int consume_ch(const char c) {
  if (*ctx.lexer.loc == c) {
    ctx.lexer.loc++; return 1;
  }
  return 0;
}

Token *lex() {
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if      ((len = consume_pred(isspace))) {}  // skip whitespace
    else if ((len = consume_ident()))       cur = (cur->next = new_token(TokenKind_IDENT, len));
    else if ((len = consume_ch('\\')))      cur = (cur->next = new_token(TokenKind_BACKSLASH, len));
    else if ((len = consume_ch('.')))       cur = (cur->next = new_token(TokenKind_DOT,       len));
    else if ((len = consume_ch('(')))       cur = (cur->next = new_token(TokenKind_LPAREN,    len));
    else if ((len = consume_ch(')')))       cur = (cur->next = new_token(TokenKind_RPAREN,    len));
    else                                    error("{@cur_loc} invalid token");
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}
