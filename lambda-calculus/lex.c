#include "lc.h"

#include <ctype.h>

int fmt_token_kind(const sink s, va_list ap) {
  const TokenKind kind = va_arg(ap, const TokenKind);
  if      (kind == TokenKind_IDENT)     return emitf(s, "ident");
  else if (kind == TokenKind_BACKSLASH) return emitf(s, "\\");
  else if (kind == TokenKind_DOT)       return emitf(s, ".");
  else if (kind == TokenKind_LPAREN)    return emitf(s, "(");
  else if (kind == TokenKind_RPAREN)    return emitf(s, ")");
  else if (kind == TokenKind_EOF)       return emitf(s, "eof");
  else                                  fail("unexpected token kind: %d", kind);
}

int fmt_token(const sink s, va_list ap) {
  const Token *tok = va_arg(ap, const Token *);
  int len = 0;
  {
    const int ret = emitf(s, "{token_kind}", tok->kind);
    if (ret < 0) return ret;
    len += ret;
  }

  if (tok->kind == TokenKind_IDENT) {
    const int ret = emitf(s, "({sv})", tok->lexeme);
    if (ret < 0) return ret;
    len += ret;
  }

  return len;
}

int fmt_token_stream(const sink s, va_list ap) {
  int len = 0;
  {
    const int ret = emitf(s, "[");
    if (ret < 0) return ret;
    len += ret;
  }

  const Token *tok = va_arg(ap, const Token *);
  while (tok) {
    {
      const int ret = emitf(s, "{token}", tok);
      if (ret < 0) return ret;
      len += ret;
    }

    if ((tok = tok->next)) {
      const int ret = emitf(s, " ");
      if (ret < 0) return ret;
      len += ret;
    }
  }

  {
    const int ret = emitf(s, "]");
    if (ret < 0) return ret;
    len += ret;
  }

  return len;
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
  tok->lexeme = sv_create(ctx.lexer.loc - len, len);
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
    else                                    error("{@cur_loc} invalid token");
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}
