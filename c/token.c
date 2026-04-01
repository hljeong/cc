#include "cc.h"

#include <stdlib.h>

static void consume_token_kind(const StrConsumer c, const TokenKind kind) {
  if      (kind == TokenKind_NUM)       consume_f(c, "num");
  else if (kind == TokenKind_IDENT)     consume_f(c, "ident");
  else if (kind == TokenKind_PLUS)      consume_f(c, "+");
  else if (kind == TokenKind_MINUS)     consume_f(c, "-");
  else if (kind == TokenKind_STAR)      consume_f(c, "*");
  else if (kind == TokenKind_SLASH)     consume_f(c, "/");
  else if (kind == TokenKind_AND)       consume_f(c, "&");
  else if (kind == TokenKind_LPAREN)    consume_f(c, "(");
  else if (kind == TokenKind_RPAREN)    consume_f(c, ")");
  else if (kind == TokenKind_LBRACKET)  consume_f(c, "[");
  else if (kind == TokenKind_RBRACKET)  consume_f(c, "]");
  else if (kind == TokenKind_EQ)        consume_f(c, "=");
  else if (kind == TokenKind_DEQ)       consume_f(c, "==");
  else if (kind == TokenKind_NEQ)       consume_f(c, "!=");
  else if (kind == TokenKind_LEQ)       consume_f(c, "<=");
  else if (kind == TokenKind_GEQ)       consume_f(c, ">=");
  else if (kind == TokenKind_LT)        consume_f(c, "<");
  else if (kind == TokenKind_GT)        consume_f(c, ">");
  else if (kind == TokenKind_SEMICOLON) consume_f(c, ";");
  else if (kind == TokenKind_COMMA)     consume_f(c, ",");
  else if (kind == TokenKind_RETURN)    consume_f(c, "return");
  else if (kind == TokenKind_LBRACE)    consume_f(c, "{");
  else if (kind == TokenKind_RBRACE)    consume_f(c, "}");
  else if (kind == TokenKind_IF)        consume_f(c, "if");
  else if (kind == TokenKind_ELSE)      consume_f(c, "else");
  else if (kind == TokenKind_FOR)       consume_f(c, "for");
  else if (kind == TokenKind_WHILE)     consume_f(c, "while");
  else if (kind == TokenKind_EOF)       consume_f(c, "eof");
  else                                  fail("unexpected token kind: %d", kind);
}

void fmt_arg_token_kind(const StrConsumer c, va_list ap) {
  consume_token_kind(c, va_arg(ap, const TokenKind));
}

static void consume_token(const StrConsumer c, const Token *tok) {
  consume_f(c, "%{token_kind}", tok->kind);
  if      (tok->kind == TokenKind_NUM)   consume_f(c, "(%d)", tok->num);
  else if (tok->kind == TokenKind_IDENT) consume_f(c, "(%{sv})", tok->ident);
}

void *fmt_ptr_token(const StrConsumer c, void *ptr) {
  consume_token(c, ptr);
  return ((const Token *) ptr)->next;
}

void fmt_arg_token(const StrConsumer c, va_list ap) {
  consume_token(c, va_arg(ap, const Token *));
}

Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv_create(ctx.lexer.loc - len, len);
  return tok;
}

Token *link(Token *tok, Token *next) {
  tok->next = next;
  next->prev = tok;
  return next;
}
