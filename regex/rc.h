typedef enum TokenKind TokenKind;
typedef struct Token Token;
typedef enum NodeKind NodeKind;
typedef struct Node Node;

enum TokenKind {
  TokenKind_PIPE,
  TokenKind_STAR,
  TokenKind_LPAREN,
  TokenKind_RPAREN,
  TokenKind_LITERAL,
  TokenKind_EOF,
};

struct Token {
  TokenKind kind;
  char c;  // TokenKind_LITERAL
  Token *next;
};

enum NodeKind {
  NodeKind_UNION,
  NodeKind_CONCAT,
  NodeKind_REPEAT,
  NodeKind_LITERAL,
};

struct Node {
  NodeKind kind;
  union {
    // NodeKind_UNION, NodeKind_CONCAT
    struct { Node *lhs, *rhs; };

    // NodeKind_REPEAT
    Node *expr;

    // NodeKind_LITERAL
    char c;
  };
};

void lex();
void parse();

typedef struct {
  const char *src;
  const Token *toks;
  const Node *ast;

  struct {
    const char *loc;
  } lexer;

  struct {
    const Token *tok;
  } parser;
} Context;

extern Context ctx;
