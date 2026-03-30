#include "cc.h"

#include <stdlib.h>

Prog *new_prog(Node *decl) {
  assert(decl->kind == NodeKind_PROG,
         "%{node_kind}", decl->kind);

  Prog *prog = calloc(1, sizeof(Prog));
  return prog;
}
