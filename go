#!/usr/bin/env bash

cd -P -- "$(dirname -- "$0")" && (
  make && \
  ./cc $@ >a.S && \
  gcc -o a.out a.S && \
  echo compiled && \
  (./a.out; echo $?)
) || make clean
