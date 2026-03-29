#include "cc.h"

const char *token_kind_to_str(const TokenKind kind) {
  if      (kind == TokenKind_NUM)       return "num";
  else if (kind == TokenKind_IDENT)     return "ident";
  else if (kind == TokenKind_PLUS)      return "+";
  else if (kind == TokenKind_MINUS)     return "-";
  else if (kind == TokenKind_STAR)      return "*";
  else if (kind == TokenKind_SLASH)     return "/";
  else if (kind == TokenKind_AND)       return "&";
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
  else if (kind == TokenKind_RETURN)    return "return";
  else if (kind == TokenKind_LBRACE)    return "{";
  else if (kind == TokenKind_RBRACE)    return "}";
  else if (kind == TokenKind_IF)        return "if";
  else if (kind == TokenKind_ELSE)      return "else";
  else if (kind == TokenKind_FOR)       return "for";
  else if (kind == TokenKind_WHILE)     return "while";
  else if (kind == TokenKind_EOF)       return "eof";
  else                                  failf("%d", kind);
}

// todo: this cant be safe... what if theres a printf with 2 token_to_str()'s?
const char *token_to_str(const Token *tok) {
  static char buf[256];
  const int off = snprintf(buf, sizeof(buf), "%s", token_kind_to_str(tok->kind));

  if (tok->kind == TokenKind_NUM) {
    snprintf(buf + off, sizeof(buf) - off, "(%d)", tok->num);
  }

  else if (tok->kind == TokenKind_IDENT) {
    snprintf(buf + off, sizeof(buf) - off, "("sv_fmt")", sv_arg(tok->ident));
  }

  return buf;
}

static void emit_token_stream(const StrConsumer consumer, void *data) {
  const Token *tok = *((const Token **) data);
  emit_s(consumer, "[[");
  do emit_f(consumer, " %s", token_to_str(tok));
  while ((tok = tok->next));
  emit_s(consumer, " ]]");
  free(data);
}

StrEmitter str_token_stream(const Token *tok) {
  assert(tok);
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
