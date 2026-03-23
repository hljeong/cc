#include "lc.h"

#include <ctype.h>

const char *token_kind_to_str(const TokenKind kind) {
  if      (kind == TokenKind_IDENT)     return "ident";
  else if (kind == TokenKind_BACKSLASH) return "\\";
  else if (kind == TokenKind_DOT)       return ".";
  else if (kind == TokenKind_LPAREN)    return "(";
  else if (kind == TokenKind_RPAREN)    return ")";
  else if (kind == TokenKind_EOF)       return "eof";
  else                                  failf("not implemented: %u",
                                              (uint32_t) kind);
}

const char *token_to_str(const Token *tok) {
  static char buf[256] = {0};
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));
  if (tok->kind == TokenKind_IDENT) {
    snprintf(buf + off, sizeof(buf) - off, "("sv_fmt")", sv_arg(tok->lexeme));
  }
  return buf;
}

void debug_token_stream(const Token *tok) {
  debugf("tokens: %s", token_to_str(tok));
  while ((tok = tok->next)) {
    debugf(" %s", token_to_str(tok));
  }
  debugf("\n");
}

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
  if (pred('\0')) fail("predicate accepts eof");
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

static Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv(ctx.lexer.loc - len, len);
  return tok;
}

Token *lex() {
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if      ((len = consume_pred(isspace))) ;  // skip whitespace
    else if ((len = consume_ident()))       cur = (cur->next = new_token(TokenKind_IDENT, len));
    else if ((len = consume_ch('\\')))      cur = (cur->next = new_token(TokenKind_BACKSLASH, len));
    else if ((len = consume_ch('.')))       cur = (cur->next = new_token(TokenKind_DOT,       len));
    else if ((len = consume_ch('(')))       cur = (cur->next = new_token(TokenKind_LPAREN,    len));
    else if ((len = consume_ch(')')))       cur = (cur->next = new_token(TokenKind_RPAREN,    len));
    else                                    errorf_loc("invalid token");
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}
