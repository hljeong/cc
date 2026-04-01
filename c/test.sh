#!/bin/bash
set -e

# todo: test expected failures

BIN=./cc

passed=0
failed=0

check() {
  desc="$1"
  shift
  expected="$1"
  shift

  compile=$($BIN "$@" >a.S 2>test.err; echo $?)
  if [ "$compile" != "0" ]; then
    echo "failed to compile: $desc"
    cat test.err
    failed=$((failed + 1))
  else
    assemble=$(gcc -x assembler -o a.out a.S 2>test.err; echo $?)
    if [ "$assemble" != "0" ]; then
      echo "failed to assemble: $desc"
      cat test.err
      failed=$((failed + 1))
    else
      actual=$(./a.out; echo $?)
      if [ "$actual" = "$expected" ]; then
        echo "pass: $desc"
        passed=$((passed + 1))
      else
        echo "incorrect output: $desc"
        echo "  expected: $expected"
        echo "  actual:   $actual"
        failed=$((failed + 1))
      fi
    fi
  fi
}

echo "=== cc tests ==="

check 'zero' '0' 'int main() {0;}'
check 'single number' '3' 'int main() {3;}'
# check 'large number' '12345' '12345;'
check 'addition' '3' 'int main() {1+2;}'
check 'subtraction' '3' 'int main() {5-2;}'
check 'multiplication' '6' 'int main() {2*3;}'
check 'division' '3' 'int main() {6/2;}'
check 'mul before add' '7' 'int main() {1+2*3;}'
check 'div before sub' '5' 'int main() {10-5/1;}'
check 'left to right add' '5' 'int main() {1+2+2;}'
check 'left to right mul' '24' 'int main() {2*3*4;}'
# check 'negation' '-5' '-5;'
check 'double negation' '5' 'int main() {--5;}'
check 'negation addition' '1' 'int main() {-1+2;}'
# check 'negation before mul' '-6' '-2*3;'
check 'eq true' '1' 'int main() {1==1;}'
check 'eq false' '0' 'int main() {1==2;}'
check 'neq true' '1' 'int main() {1!=2;}'
check 'neq false' '0' 'int main() {1!=1;}'
check 'lt true' '1' 'int main() {1<2;}'
check 'lt false' '0' 'int main() {2<1;}'
check 'gt true' '1' 'int main() {2>1;}'
check 'gt false' '0' 'int main() {1>2;}'
check 'leq true' '1' 'int main() {1<=2;}'
check 'leq false' '0' 'int main() {3<=2;}'
check 'geq true' '1' 'int main() {2>=1;}'
check 'geq false' '0' 'int main() {1>=2;}'
check 'assign and read' '5' 'int main() {int a=5;a;}'
check 'assign two vars' '10' 'int main() {int a=5;int b=10;b;}'
check 'chain assign' '10' 'int main() {int a; int b; a=b=10;a;}'
check 'long variable name' '10' 'int main() {int abc; int bcd; abc=bcd=10;abc;}'
check 'grouped add' '9' 'int main() {(1+2)*3;}'
check 'nested parens' '4' 'int main() {(1+(2*3))-3;}'
check 'parens precedence' '7' 'int main() {(1+2)*3-2;}'
check 'return' '1' 'int main() {return 1;}'
check 'return early' '3' 'int main() {return 3; 2; 1;}'
check 'return not-so-early' '2' 'int main() {1; return 2; 3;}'
check 'block in block' '2' 'int main() {1; {int x = 5; return 2;} 3;}'
check 'empty block' '0' 'int main() {{} 0;}'
check 'empty statement' '3' 'int main() { int _; ;; ;_; return 3 ;_; ;; }'
check 'if' '3' 'int main() { if (2 < 3) return 3; return 2; }'
check 'if else' '2' 'int main() { if (1 + 1 > 3) return 3; else { return 2; } }'
check 'for' '45' 'int main() { int i = 0; int j = 0; for (i = 0; i < 10; i = i + 1) j = i + j; return j; }'
check 'forever' '1' 'int main() { for (;;) return 1; return 2; }'
check 'while' '15' 'int main() { int i = 0; while (i < 15) i = i + 1; return i; }'
check 'addr-deref' '3' 'int main() { int x = 3; return *&x; }'
check 'pointer' '2' 'int main() { int x = 3; int *y = &x; *y = 2; return x; }'
check 'double pointer' '154' 'int main() { int x = 3; int y = 5; int *z = &y; int **w = &z; **w = 7; *w = &x; **w = 11; return x * (y + y); }'
check 'pointer plus' '4' 'int main() { int y = 5; int x = 3; *(&y + 1) = 4; return x; }'
check 'pointer sub' '4' 'int main() { int y = 5; int x = 3; *(&x - 1) = 4; return y; }'
check 'pointer dist' '1' 'int main() { int y = 5; int x = 3; return &x - &y; }'
check 'pointer arithmetic' '6' 'int main() { int y = 5; int x = 3; *(&y - 20 + 3 * 7) = y; return x + (&x - &y); }'
check 'pointer arithmetic' '6' 'int main() { int y = 5; int x = 3; *(&y - 20 + 3 * 7) = y; return x + (&x - &y); }'
check 'multiple declarations' '14' 'int main() { int x = 3, y = 5; return x * y - 1; }'
check 'refer to declared vars in same stmt' '3' 'int main() { int x = 3, *y = &x, z = *y; z; }'
check 'function call' '1' 'int f() { return 1; } int main() { return f(); }'
check 'function call with 1 arg' '3' 'int f(int x) { return x; } int main() { return f(3); }'
check 'function call with args' '1' 'int f(int x, int y, int z) { return x; } int main() { int x = 1, y = 2, z = 3; return f(x, y, z); }'
check 'function call with diverse args' '2' 'int f(int x, int y, int *z) { if (*z == x) return x; else return y; } int main() { int x = 1, y = 2, *z = &y; return f(x, y, z); }'
check 'factorial' '120' 'int fact(int n) { if (n == 0) return 1; else return n * fact(n - 1); } int main() { return fact(5); }'
check 'array 0' '1' 'int main() { int arr[3]; *(arr + 0) = 1; return *(arr + 0); }'
check 'array 1' '2' 'int main() { int arr[3]; *(arr + 1) = 2; return arr[1]; }'
check 'array 2' '3' 'int main() { int arr[3]; arr[2] = 3; return *(arr + 2); }'
check 'multi-dim array 0' '1' 'int main() { int arr[2][3], *flat = arr; *(flat + 0) = 1; return *(*(arr + 0) + 0); }'
check 'multi-dim array 1' '2' 'int main() { int arr[2][3], *flat = arr; *(flat + 1) = 2; return arr[0][1]; }'
check 'multi-dim array 2' '3' 'int main() { int arr[2][3], *flat = arr; arr[0][2] = 3; return *(*(arr + 0) + 2); }'
check 'multi-dim array 3' '4' 'int main() { int arr[2][3], *flat = arr; arr[1][0] = 4; return *(*(arr + 1) + 0); }'
check 'multi-dim array 4' '5' 'int main() { int arr[2][3], *flat = arr; *(flat + 4) = 5; return arr[1][1]; }'
check 'multi-dim array 5' '6' 'int main() { int arr[2][3], *flat = arr; *(flat + 5) = 6; return *(*(arr + 1) + 2); }'
check 'sizeof 0' '16' 'int main() { int x; return sizeof(x) + sizeof x; }'  # death penalty
check 'sizeof 1' '8' 'int main() { int *x; return sizeof(x); }'
check 'sizeof 2' '32' 'int main() { int x[4]; return sizeof(x); }'
check 'sizeof 3' '96' 'int main() { int x[3][4]; return sizeof(x); }'
check 'sizeof 4' '32' 'int main() { int x[3][4]; return sizeof(*x); }'
check 'sizeof 5' '8' 'int main() { int x[3][4]; return sizeof(**x); }'
check 'sizeof 6' '33' 'int main() { int x[3][4]; return sizeof *x + 1; }'
check 'sizeof 7' '8' 'int main() { int x = 1; return sizeof(x = 2); }'
check 'sizeof 8' '1' 'int main() { int x = 1; sizeof(x = 2); return x; }'

echo ""
echo "=== results: $passed passed, $failed failed ==="

if [ $failed -gt 0 ]; then
  exit 1
fi
