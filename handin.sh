#!/bin/bash

if [[ -n $(git status -s) ]]; then
  echo "[*_*]: You have uncommitted change, please commit your codes before handing in."
  echo ""
  git status -s
  echo ""
  echo "\$ git add somefiles"
  echo "\$ git commit -m \"a message\""
  exit 1
fi

tar czf labx_xxx.tar.gz --exclude=lex.cc --exclude=scannerbase.h --exclude=parse.cc --exclude=parserbase.h src
echo "Please rename labx_xxx.tar.gz to lab<lab number>_<your student id>.tar.gz"
echo "For example, lab1_517030900000.tar.gz."
