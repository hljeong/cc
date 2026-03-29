#include "cc.h"

StrEmitter str_token_kind(const TokenKind kind) {
  if      (kind == TokenKind_NUM)       return str_f("num");
  else if (kind == TokenKind_IDENT)     return str_f("ident");
  else if (kind == TokenKind_PLUS)      return str_f("+");
  else if (kind == TokenKind_MINUS)     return str_f("-");
  else if (kind == TokenKind_STAR)      return str_f("*");
  else if (kind == TokenKind_SLASH)     return str_f("/");
  else if (kind == TokenKind_AND)       return str_f("&");
  else if (kind == TokenKind_LPAREN)    return str_f("(");
  else if (kind == TokenKind_RPAREN)    return str_f(")");
  else if (kind == TokenKind_EQ)        return str_f("=");
  else if (kind == TokenKind_DEQ)       return str_f("==");
  else if (kind == TokenKind_NEQ)       return str_f("!=");
  else if (kind == TokenKind_LEQ)       return str_f("<=");
  else if (kind == TokenKind_GEQ)       return str_f(">=");
  else if (kind == TokenKind_LT)        return str_f("<");
  else if (kind == TokenKind_GT)        return str_f(">");
  else if (kind == TokenKind_SEMICOLON) return str_f(";");
  else if (kind == TokenKind_RETURN)    return str_f("return");
  else if (kind == TokenKind_LBRACE)    return str_f("{");
  else if (kind == TokenKind_RBRACE)    return str_f("}");
  else if (kind == TokenKind_IF)        return str_f("if");
  else if (kind == TokenKind_ELSE)      return str_f("else");
  else if (kind == TokenKind_FOR)       return str_f("for");
  else if (kind == TokenKind_WHILE)     return str_f("while");
  else if (kind == TokenKind_EOF)       return str_f("eof");
  else                                  fail(str_int(kind));
}

static void emit_token(const StrConsumer consumer, void *data) {
  const Token *tok = *((const Token **) data);
  emit_e(consumer, str_token_kind(tok->kind));

  if      (tok->kind == TokenKind_NUM)   emit_f(consumer, "(%d)", tok->num);
  else if (tok->kind == TokenKind_IDENT) emit_f(consumer, "("sv_fmt")", sv_arg(tok->ident));

  free(data);
}

StrEmitter str_token(const Token *tok) {
  const Token **tok_ptr = calloc(1, sizeof(const Token *));
  *tok_ptr = tok;
  return (StrEmitter) { .emit = emit_token, .data = tok_ptr };
}

static void emit_token_stream(const StrConsumer consumer, void *data) {
  const Token *tok = *((const Token **) data);
  emit_s(consumer, "[[");
  do {
    emit_s(consumer, " ");
    emit_e(consumer, str_token(tok));
  } while ((tok = tok->next));
  emit_s(consumer, " ]]");
  free(data);
}

StrEmitter str_token_stream(const Token *tok) {
  assert(tok, "todo: allow raw asserts");
  const Token **tok_ptr = calloc(1, sizeof(const Token **));
  *tok_ptr = tok;
  return (StrEmitter) { .emit = emit_token_stream, .data = tok_ptr };
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
