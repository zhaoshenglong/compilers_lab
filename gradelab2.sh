#!/bin/bash

BIN=test_lex
REFOUTDIR=refs
TESTCASEDIR=testcases

base_name=$(basename "$PWD")
if [[ ! $base_name =~ "tiger-compiler" ]]; then
	echo "[-_-]: Not in Lab Root Dir"
	echo "SCORE: 0"
	exit 1
fi
mkdir -p build
cd build
rm -f testcases refs _tmp.txt .tmp.txt
ln -s ../testdata/lab2/testcases testcases
ln -s ../testdata/lab2/refs refs
cmake .. >&/dev/null
make ${BIN} -j >/dev/null
if [[ $? != 0 ]]; then
  echo "[-_-]$ite: Compile Error"
  echo "TOTAL SCORE: 0"
  exit 123
fi

for tcase in $(ls $TESTCASEDIR); do
  if [ ${tcase##*.} = "tig" ]; then
    tfileName=${tcase##*/}

    ./$BIN $TESTCASEDIR/$tfileName >&_tmp.txt
    diff $DIFFOPTION _tmp.txt $REFOUTDIR/${tfileName%.*}.out >&.tmp.txt
    if [ -s .tmp.txt ]; then
      cat .tmp.txt
      echo "[*_*]$ite: Output Mismatch [$tfileName]"
      rm -f _tmp.txt .tmp.txt
      echo "TOTAL SCORE: 0"
      exit 234
    fi
  fi
done

rm -f testcases refs _tmp.txt .tmp.txt
echo "[^_^]$ite: Pass"
echo "TOTAL SCORE: 100"
