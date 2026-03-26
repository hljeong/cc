#include "cc.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

StringView sv(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

const char *token_kind_to_str(const TokenKind kind) {
  if      (kind == TokenKind_NUM)       return "num";
  else if (kind == TokenKind_IDENT)     return "ident";
  else if (kind == TokenKind_PLUS)      return "+";
  else if (kind == TokenKind_MINUS)     return "-";
  else if (kind == TokenKind_STAR)      return "*";
  else if (kind == TokenKind_SLASH)     return "/";
  else if (kind == TokenKind_LPAREN)    return "(";
  else if (kind == TokenKind_RPAREN)    return ")";
  else if (kind == TokenKind_EQ)        return "=";
  else if (kind == TokenKind_DEQ)       return "==";
  else if (kind == TokenKind_NEQ)       return "!=";
  else if (kind == TokenKind_LEQ)       return "<=";
  else if (kind == TokenKind_GEQ)       return ">=";
  else if (kind == TokenKind_LT)        return "<";
  else if (kind == TokenKind_GT)        return ">";
  else if (kind == TokenKind_SEMICOLON) return ";";
  else if (kind == TokenKind_EOF)       return "eof";
  else                                  failf("not implemented: %u",
                                              (uint32_t) kind);
}

const char *token_to_str(const Token *tok) {
  static char buf[256] = {0};
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));

  if (tok->kind == TokenKind_NUM) {
    snprintf(buf + off, sizeof(buf) - off, "(%d)", tok->num);
  }

  else if (tok->kind == TokenKind_IDENT) {
    snprintf(buf + off, sizeof(buf) - off, "("sv_fmt")", sv_arg(tok->ident));
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

static int consume_pred(int (*pred)(int)) {
  assertm(!pred('\0'), "predicate accepts eof");
  const char *start = ctx.lexer.loc;
  while (pred(*ctx.lexer.loc)) ctx.lexer.loc++;
  return ctx.lexer.loc - start;
}

static int consume_ident() {
  int len = 0;
  if (!(len = consume_pred(is_ident_first))) return 0;
  return (len += consume_pred(is_ident_rest));
}

// if current loc matches ch:
// - return 1
// - advance current loc by 1
// otherwise return 0
static int consume_ch(const char ch) {
  if (*ctx.lexer.loc == ch) {
    ctx.lexer.loc++;
    return 1;
  }
  return 0;
}

// if current loc matches s:
// - return strlen(s)
// - advance current loc by strlen(s)
// otherwise return 0
static int consume(const char *s) {
  const int len = strlen(s);
  assertm(len > 0, "empty literal");
  if (!strncmp(ctx.lexer.loc, s, len)) {
    ctx.lexer.loc += len;
    return len;
  } else return 0;
}

// consume_ch(ch), except fail if unsuccesful
static int expect_ch(const char ch) {
  const int len = consume_ch(ch);
  if (!len) errorf_loc("expected '%c'", ch);
  return len;
}

// consume(s), except fail if unsuccesful
static int expect(const char *s) {
  const int len = consume(s);
  if (!len) errorf_loc("expected \"%s\"");
  return len;
}

static Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv(ctx.lexer.loc - len, len);
  return tok;
}

static Token *link(Token *tok, Token *next_tok) {
  tok->next = next_tok;
  next_tok->prev = tok;
  return next_tok;
}

Token *lex() {
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if (consume_pred(isspace)) ;  // skip whitespace

    else if (isdigit(ch)) {
      const char *start = ctx.lexer.loc;
      char *end = NULL;
      const int num = strtoul(ctx.lexer.loc, &end, 10);
      ctx.lexer.loc = end;
      cur = link(cur, new_token(TokenKind_NUM, ctx.lexer.loc - start));
      cur->num = num;
    }

    else if ((len = consume_ident())) {
      cur = link(cur, new_token(TokenKind_IDENT, len));
      cur->ident = cur->lexeme;
    }

    else if ((len = consume("==")))   cur = link(cur, new_token(TokenKind_DEQ,       len));
    else if ((len = consume("!=")))   cur = link(cur, new_token(TokenKind_NEQ,       len));
    else if ((len = consume("<=")))   cur = link(cur, new_token(TokenKind_LEQ,       len));
    else if ((len = consume(">=")))   cur = link(cur, new_token(TokenKind_GEQ,       len));
    else if ((len = consume_ch('<'))) cur = link(cur, new_token(TokenKind_LT,        len));
    else if ((len = consume_ch('>'))) cur = link(cur, new_token(TokenKind_GT,        len));
    else if ((len = consume_ch('+'))) cur = link(cur, new_token(TokenKind_PLUS,      len));
    else if ((len = consume_ch('-'))) cur = link(cur, new_token(TokenKind_MINUS,     len));
    else if ((len = consume_ch('*'))) cur = link(cur, new_token(TokenKind_STAR,      len));
    else if ((len = consume_ch('/'))) cur = link(cur, new_token(TokenKind_SLASH,     len));
    else if ((len = consume_ch('('))) cur = link(cur, new_token(TokenKind_LPAREN,    len));
    else if ((len = consume_ch(')'))) cur = link(cur, new_token(TokenKind_RPAREN,    len));
    else if ((len = consume_ch(';'))) cur = link(cur, new_token(TokenKind_SEMICOLON, len));
    else if ((len = consume_ch('='))) cur = link(cur, new_token(TokenKind_EQ,        len));
    else                              errorf_loc("invalid token");
  }

  cur = link(cur, new_token(TokenKind_EOF, 0));
  return head.next;
}
