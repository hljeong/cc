#include "cc.h"

#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int is_ident_first(int c) {
  return isalpha(c) || (c == '_');
}

static int is_ident_rest(int c) {
  return isalnum(c) || (c == '_');
}

static int consume_pred(int (*pred)(int)) {
  assert_f(!pred('\0'), "predicate accepts eof");
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
  assert_f(len > 0, "empty literal");
  if (!strncmp(ctx.lexer.loc, s, len)) {
    ctx.lexer.loc += len;
    return len;
  } else return 0;
}

// consume_ch(ch), except fail if unsuccesful
static int expect_ch(const char ch) {
  const int len = consume_ch(ch);
  if (!len) error_f("%{@cur_loc} expected '%c'", ch);
  return len;
}

// consume(s), except fail if unsuccesful
static int expect(const char *s) {
  const int len = consume(s);
  if (!len) error_f("%{@cur_loc} expected \"%s\"", s);
  return len;
}

void lex() {
  ctx.lexer.loc = ctx.src.loc;
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if (consume_pred(isspace)) ;  // skip whitespace

    else if ((len = consume("return"))) cur = link(cur, new_token(TokenKind_RETURN, len));
    else if ((len = consume("if")))     cur = link(cur, new_token(TokenKind_IF,     len));
    else if ((len = consume("else")))   cur = link(cur, new_token(TokenKind_ELSE,   len));
    else if ((len = consume("for")))    cur = link(cur, new_token(TokenKind_FOR,    len));
    else if ((len = consume("while")))  cur = link(cur, new_token(TokenKind_WHILE,  len));

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
    else if ((len = consume_ch('&'))) cur = link(cur, new_token(TokenKind_AND,       len));
    else if ((len = consume_ch('('))) cur = link(cur, new_token(TokenKind_LPAREN,    len));
    else if ((len = consume_ch(')'))) cur = link(cur, new_token(TokenKind_RPAREN,    len));
    else if ((len = consume_ch(';'))) cur = link(cur, new_token(TokenKind_SEMICOLON, len));
    else if ((len = consume_ch('='))) cur = link(cur, new_token(TokenKind_EQ,        len));
    else if ((len = consume_ch('{'))) cur = link(cur, new_token(TokenKind_LBRACE,    len));
    else if ((len = consume_ch('}'))) cur = link(cur, new_token(TokenKind_RBRACE,    len));
    else                              error_f("%{@cur_loc} invalid token");
  }

  cur = link(cur, new_token(TokenKind_EOF, 0));
  ctx.toks = head.next;
}
