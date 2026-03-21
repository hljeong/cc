#include "cc.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>

StringView sv(const char *loc, const int len) {
  return (StringView) { .loc = loc, .len = len };
}

const char *token_kind_to_str(const TokenKind kind) {
  if      (kind == TokenKind_ADD)    return "+";
  else if (kind == TokenKind_SUB)    return "-";
  else if (kind == TokenKind_MUL)    return "*";
  else if (kind == TokenKind_DIV)    return "/";
  else if (kind == TokenKind_LPAREN) return "(";
  else if (kind == TokenKind_RPAREN) return ")";
  else if (kind == TokenKind_EEQ)    return "==";
  else if (kind == TokenKind_NEQ)    return "!=";
  else if (kind == TokenKind_LEQ)    return "<=";
  else if (kind == TokenKind_GEQ)    return ">=";
  else if (kind == TokenKind_LT)     return "<";
  else if (kind == TokenKind_GT)     return ">";
  else if (kind == TokenKind_NUM)    return "number";
  else if (kind == TokenKind_EOF)    return "eof";
  else                               failf("not implemented: %u",
                                           (uint32_t) kind);
}

const char *token_to_str(const Token *tok) {
  static char buf[100] = {0};
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));

  if (tok->kind == TokenKind_NUM) {
    snprintf(buf + off, sizeof(buf) - off, "(%d)", tok->num);
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
  return strncmp(ctx.lexer.loc, s, len) ? 0 : len;
}

// lex_consume_ch(ch), except fail if unsuccesful
static int expect_ch(const char ch) {
  const int len = consume_ch(ch);
  if (!len) errorf_loc("expected '%c'", ch);
  return len;
}

// lex_consume(s), except fail if unsuccesful
static int expect(const char *s) {
  const int len = consume(s);
  if (!len) errorf_loc("expected \"%s\"");
  return len;
}

static Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv(ctx.lexer.loc, len);
  return tok;
}

Token *lex() {
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if (isspace(ch)) ctx.lexer.loc++;

    else if (isdigit(ch)) {
      cur = (cur->next = new_token(TokenKind_NUM, 0));
      const char *start = ctx.lexer.loc;
      // todo: why is this cast needed?
      cur->num = strtoul(ctx.lexer.loc, (char **) &ctx.lexer.loc, 10);
      cur->lexeme.len = ctx.lexer.loc - start;
    }

    else if ((len = consume("==")))   cur = (cur->next = new_token(TokenKind_EEQ,    len));
    else if ((len = consume("!=")))   cur = (cur->next = new_token(TokenKind_NEQ,    len));
    else if ((len = consume("<=")))   cur = (cur->next = new_token(TokenKind_LEQ,    len));
    else if ((len = consume(">=")))   cur = (cur->next = new_token(TokenKind_GEQ,    len));
    else if ((len = consume_ch('<'))) cur = (cur->next = new_token(TokenKind_LT,     len));
    else if ((len = consume_ch('>'))) cur = (cur->next = new_token(TokenKind_GT,     len));
    else if ((len = consume_ch('+'))) cur = (cur->next = new_token(TokenKind_ADD,    len));
    else if ((len = consume_ch('-'))) cur = (cur->next = new_token(TokenKind_SUB,    len));
    else if ((len = consume_ch('*'))) cur = (cur->next = new_token(TokenKind_MUL,    len));
    else if ((len = consume_ch('/'))) cur = (cur->next = new_token(TokenKind_DIV,    len));
    else if ((len = consume_ch('('))) cur = (cur->next = new_token(TokenKind_LPAREN, len));
    else if ((len = consume_ch(')'))) cur = (cur->next = new_token(TokenKind_RPAREN, len));
    else                              errorf_loc("invalid token");
  }

  cur = (cur->next = new_token(TokenKind_EOF, 0));
  return head.next;
}
