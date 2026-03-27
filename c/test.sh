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

check 'zero' '0' '{0;}'
check 'single number' '3' '{3;}'
# check 'large number' '12345' '12345;'
check 'addition' '3' '{1+2;}'
check 'subtraction' '3' '{5-2;}'
check 'multiplication' '6' '{2*3;}'
check 'division' '3' '{6/2;}'
check 'mul before add' '7' '{1+2*3;}'
check 'div before sub' '5' '{10-5/1;}'
check 'left to right add' '5' '{1+2+2;}'
check 'left to right mul' '24' '{2*3*4;}'
# check 'negation' '-5' '-5;'
check 'double negation' '5' '{--5;}'
check 'negation addition' '1' '{-1+2;}'
# check 'negation before mul' '-6' '-2*3;'
check 'eq true' '1' '{1==1;}'
check 'eq false' '0' '{1==2;}'
check 'neq true' '1' '{1!=2;}'
check 'neq false' '0' '{1!=1;}'
check 'lt true' '1' '{1<2;}'
check 'lt false' '0' '{2<1;}'
check 'gt true' '1' '{2>1;}'
check 'gt false' '0' '{1>2;}'
check 'leq true' '1' '{1<=2;}'
check 'leq false' '0' '{3<=2;}'
check 'geq true' '1' '{2>=1;}'
check 'geq false' '0' '{1>=2;}'
check 'assign and read' '5' '{a=5;a;}'
check 'assign two vars' '10' '{a=5;b=10;b;}'
check 'chain assign' '10' '{a=b=10;a;}'
check 'long variable name' '10' '{abc=bcd=10;abc;}'
check 'grouped add' '9' '{(1+2)*3;}'
check 'nested parens' '4' '{(1+(2*3))-3;}'
check 'parens precedence' '7' '{(1+2)*3-2;}'
check 'return' '1' '{return 1;}'
check 'return early' '3' '{return 3; 2; 1;}'
check 'return not-so-early' '2' '{1; return 2; 3;}'
check 'block in block' '2' '{1; {x = 5; return 2;} 3;}'
check 'empty block' '0' '{{} 0;}'
check 'empty statement' '3' '{ ;; ;_; return 3 ;_; ;; }'
check 'if' '3' '{ if (2 < 3) return 3; return 2; }'
check 'if else' '2' '{ if (1 + 1 > 3) return 3; else { return 2; } }'
check 'for' '45' '{ i = 0; j = 0; for (i = 0; i < 10; i = i + 1) j = i + j; return j; }'
check 'forever' '1' '{ for (;;) return 1; return 2; }'
check 'while' '15' '{ i = 0; while (i < 15) i = i + 1; return i; }'
check 'addr-deref' '3' '{ x = 3; return *&x; }'
check 'pointer' '2' '{ x = 3; y = &x; *y = 2; return x; }'
check 'double pointer' '154' '{ x = 3; y = 5; z = &y; w = &z; **w = 7; *w = &x; **w = 11; return x * (y + y); }'
check 'pointer plus' '4' '{ x = 3; y = 5; *(&y + 1) = 4; return x; }'
check 'pointer sub' '4' '{ x = 3; y = 5; *(&x - 1) = 4; return y; }'
check 'pointer dist' '1' '{ x = 3; y = 5; return &x - &y; }'
check 'pointer arithmetic' '6' '{ x = 3; y = 5; *(&y - 20 + 3 * 7) = y; return x + (&x - &y); }'

echo ""
echo "=== results: $passed passed, $failed failed ==="

if [ $failed -gt 0 ]; then
  exit 1
fi
