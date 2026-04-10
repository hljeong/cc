#include "lc.h"

int fmt_token_kind(const sink s, va_list ap) {
  const TokenKind kind = va_arg(ap, const TokenKind);
  if      (kind == TokenKind_IDENT)     return emitf(s, "ident");
  else if (kind == TokenKind_BACKSLASH) return emitf(s, "\\");
  else if (kind == TokenKind_DOT)       return emitf(s, ".");
  else if (kind == TokenKind_LPAREN)    return emitf(s, "(");
  else if (kind == TokenKind_RPAREN)    return emitf(s, ")");
  else if (kind == TokenKind_LET)       return emitf(s, "let");
  else if (kind == TokenKind_WALRUS)    return emitf(s, ":=");
  else if (kind == TokenKind_SEMICOLON) return emitf(s, ";");
  else if (kind == TokenKind_EOF)       return emitf(s, "eof");
  else                                  fail("unexpected token kind: %d", kind);
}

int fmt_token(const sink s, va_list ap) {
  const Token *tok = va_arg(ap, const Token *);
  int len = 0;
  len += check(emitf(s, "{token_kind}", tok->kind));

  if (tok->kind == TokenKind_IDENT) {
    len += check(emitf(s, "({sv})", tok->lexeme));
  }

  return len;
}

int fmt_token_stream(const sink s, va_list ap) {
  int len = check(emitf(s, "["));

  const Token *tok = va_arg(ap, const Token *);
  while (tok) {
    len += check(emitf(s, "{token}", tok));

    if ((tok = tok->next)) {
      len += check(emitf(s, " "));
    }
  }

  return (len += check(emitf(s, "]")));
}

Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv_create(ctx.lexer.loc - len, len);
  return tok;
}
