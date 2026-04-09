#include "lc.h"

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

Token *new_token(const TokenKind kind, const int len) {
  Token *tok = calloc(1, sizeof(Token));
  tok->kind = kind;
  tok->lexeme = sv_create(ctx.lexer.loc - len, len);
  return tok;
}
