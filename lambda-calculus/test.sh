#!/bin/bash
set -e

BIN=./lc

passed=0
failed=0

check() {
  desc="$1"
  shift
  expected="$1"
  shift

  actual=$($BIN "$@" 2>/dev/null)
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

echo "=== lambda-calculus tests ==="

check "identity"                                    '(\x.x)'                '\x.x'
check "simple beta reduction"                       '(\a.a)'                '(\x.x)(\a.a)'
check "simple beta reduction 2"                     '(\b.b)'                '(\x.\y.y)(\a.a)(\b.b)'
check "nested function application"                 '(\a.a)'                '(\x.\y.x)((\z.z)(\a.a))(\b.b)'
check "whnf: identity is already in whnf"           '(\x.x)'                '(\x.x)'                          whnf
check "whnf: stops before reducing inside lambda"   '(\y.((\z.z) y))'       '(\x.\y.((\z.z) y))(\a.a)'        whnf
check "whnf: stops reducing at application"         '(y ((\x.x) y))'        '(\x.y x)((\x.x) y)'              whnf
check "nf: reduces inside lambda"                   '(\y.y)'                '(\x.\y.((\z.z) y))(\a.a)'        nf
check "nf: reduces inside value"                    '(y y)'                 '((\x.x) y)((\x.x) y)'            nf
check "max-steps: limits reductions"                '((\y.(\a.a)) (\b.b))'  '(\x.\y.x)(\a.a)(\b.b)'           nf    1
check "shadow: inner binding hides outer"           'b'                     '(\x.\x.x) a b'
check "shadow: prevents free var capture"           'y'                     '(\x.\y.x) y b'


echo ""
echo "=== results: $passed passed, $failed failed ==="

if [ $failed -gt 0 ]; then
  exit 1
fi
