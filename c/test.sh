#!/bin/bash
set -e

BIN=./cc

passed=0
failed=0

check() {
  desc="$1"
  shift
  expected="$1"
  shift

  actual=$(gcc -x assembler -o a.out <($BIN "$@" 2>/dev/null) && ./a.out; echo $?)
  if [ "$actual" = "$expected" ]; then
    echo "pass: $desc"
    passed=$((passed + 1))
  else
    echo "fail: $desc"
    echo "  expected: $expected"
    echo "  actual:   $actual"
    failed=$((failed + 1))
  fi
}

echo "=== cc tests ==="

echo "--- numbers ---"
check 'zero' '0' '0;'
check 'single number' '3' '3;'
# check 'large number' '12345' '12345;'

echo "--- arithmetic ---"
check 'addition' '3' '1+2;'
check 'subtraction' '3' '5-2;'
check 'multiplication' '6' '2*3;'
check 'division' '3' '6/2;'

echo "--- precedence ---"
check 'mul before add' '7' '1+2*3;'
check 'div before sub' '5' '10-5/1;'
check 'left to right add' '5' '1+2+2;'
check 'left to right mul' '24' '2*3*4;'

echo "--- unary ---"
# check 'negation' '-5' '-5;'
check 'double negation' '5' '--5;'
check 'negation addition' '1' '-1+2;'
# check 'negation before mul' '-6' '-2*3;'

echo "--- comparisons ---"
check 'eq true' '1' '1==1;'
check 'eq false' '0' '1==2;'
check 'neq true' '1' '1!=2;'
check 'neq false' '0' '1!=1;'
check 'lt true' '1' '1<2;'
check 'lt false' '0' '2<1;'
check 'gt true' '1' '2>1;'
check 'gt false' '0' '1>2;'
check 'leq true' '1' '1<=2;'
check 'leq false' '0' '3<=2;'
check 'geq true' '1' '2>=1;'
check 'geq false' '0' '1>=2;'

echo "--- variables ---"
check 'assign and read' '5' 'a=5;a;'
check 'assign two vars' '10' 'a=5;b=10;b;'
check 'chain assign' '10' 'a=b=10;a;'
check 'long variable name' '10' 'abc=bcd=10;abc;'

echo "--- parentheses ---"
check 'grouped add' '9' '(1+2)*3;'
check 'nested parens' '4' '(1+(2*3))-3;'
check 'parens precedence' '7' '(1+2)*3-2;'

echo ""
echo "=== results: $passed passed, $failed failed ==="

if [ $failed -gt 0 ]; then
  exit 1
fi
