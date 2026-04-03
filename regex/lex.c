#include "rc.h"

#include <stdbool.h>
#include <stdlib.h>

static bool consume(const char c) {
  if (*ctx.lexer.loc == c) return ++ctx.lexer.loc;
  return false;
}

static Token *new_token(const TokenKind kind, const char c) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->c = c;
  return tok;
}

void lex() {
  ctx.lexer.loc = ctx.src;
  Token head = {};
  Token *cur = &head;

  int len = 0;
  char ch = '\0';
  while ((ch = *ctx.lexer.loc)) {
    if      (consume('|'))  cur = (cur->next = new_token(TokenKind_PIPE, '|'));
    else if (consume('*'))  cur = (cur->next = new_token(TokenKind_STAR, '*'));
    else if (consume('('))  cur = (cur->next = new_token(TokenKind_LPAREN, '('));
    else if (consume(')'))  cur = (cur->next = new_token(TokenKind_RPAREN, ')'));

    else {
      consume('\\');
      cur = (cur->next = new_token(TokenKind_LITERAL, *ctx.lexer.loc++));
    }
  }

  cur->next = new_token(TokenKind_EOF, '\0');
  ctx.toks = head.next;
}
