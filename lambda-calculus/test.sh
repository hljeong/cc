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
  input="$1"
  shift

  actual=$(echo "$input" | $BIN - "$@" 2>/dev/null)
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

check "identity"                                    '\x.x'                '\x.x'
check "simple beta reduction"                       '\a.a'                '(\x.x)(\a.a)'
check "simple beta reduction 2"                     '\b.b'                '(\x.\y.y)(\a.a)(\b.b)'
check "nested function application"                 '\a.a'                '(\x.\y.x)((\z.z)(\a.a))(\b.b)'
check "whnf: identity is already in whnf"           '\x.x'                '(\x.x)'                          whnf
check "whnf: stops before reducing inside lambda"   '\y.(\z.z) y'         '(\x.\y.((\z.z) y))(\a.a)'        whnf
check "whnf: stops reducing at application"         'y ((\x.x) y)'        '(\x.y x)((\x.x) y)'              whnf
check "nf: reduces inside lambda"                   '\y.y'                '(\x.\y.((\z.z) y))(\a.a)'        nf
check "nf: reduces inside value"                    'y y'                 '((\x.x) y)((\x.x) y)'            nf
check "nf: does not eta-reduce"                     '\x.g x'              '(\f.(\x.g (f x)))(\y.y)'         nf
check "benf"                                        'g'                   '(\f.\x.(f x)) g'                 benf
check "benf: beta-eta combo"                        'g'                   '(\f.(\x.g (f x)))(\y.y)'         benf
check "benf: does not faux reduce"                  '\x.x x'              '(\x.x x)'                        benf
check "max-steps: limits reductions"                '(\y.\a.a) \b.b'      '(\x.\y.x)(\a.a)(\b.b)'           nf    1
check "shadow: inner binding hides outer"           'b'                   '(\x.\x.x) a b'
check "shadow: prevents free var capture"           'y'                   '(\x.\y.x) y b'

check "extended syntax 0"                           '\x.\y.x'             '\x y.x'
check "extended syntax 1"                           '\x y.x'              '\x y.x'                          nf    100 ext
check "extended syntax 2"                           '\f x.f (f (f x))'    'let succ:=\n f x.n f (f x); (succ (succ (succ \f x.x)))' nf 100 ext
check "odd identifiers"                             '\f x.f (f (f (f x)))'    'let 0 := \f x.x; let succ:=\n f x.n f (f x); let 1 := succ 0; let 2 := succ 1; let + := \n m.m succ n; + 2 2' nf 100 ext


echo ""
echo "=== results: $passed passed, $failed failed ==="

if [ $failed -gt 0 ]; then
  exit 1
fi
