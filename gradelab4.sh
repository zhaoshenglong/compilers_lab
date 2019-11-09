#!/bin/bash
#============some output color
SYS=$(uname -s)
if [[ $SYS == "Linux" ]]; then
  RED_COLOR='\E[1;31m'
  GREEN_COLOR='\E[1;32m'
  YELOW_COLOR='\E[1;33m'
  BLUE_COLOR='\E[1;34m'
  PINK='\E[1;35m'
  RES='\E[0m'
fi

BIN=test_semant
#PROJDIR=lab4
REFOUTDIR=refs
TESTCASEDIR=testcases
DIFFOPTION="-w -B"
ret_value=0

base_name=$(basename "$PWD")
if [[ ! $base_name =~ "tiger-compiler" ]]; then
  echo "[-_-]: Not in Lab Root Dir"
  echo "SCORE: 0"
  exit 1
fi

mkdir -p build
cd build
rm -f testcases refs _tmp.txt .tmp.txt __tmp.txt _ref.txt
rm -f testcases refs _tmp.txt .tmp.txt
ln -s ../testdata/lab4/testcases testcases
if [[ $? != 0 ]]; then
  echo "[-_-]$ite: Link Error"
  echo "TOTAL SCORE: 0"
  exit 123
fi

ln -s ../testdata/lab4/refs refs
if [[ $? != 0 ]]; then
  echo "[-_-]$ite: Link Error"
  echo "TOTAL SCORE: 0"
  exit 123
fi

cmake .. >&/dev/null
make ${BIN} -j >/dev/null
if [[ $? != 0 ]]; then
  echo -e "${RED_COLOR}[-_-]$ite: Compile Error${RES}"
  exit 123
fi
for tcase in $(ls $TESTCASEDIR/); do
  if [ ${tcase##*.} = "tig" ]; then
    tfileName=${tcase##*/}
    #echo $tfileName
    rm -f _ref.txt _tmp.txt .tmp.txt
    ./$BIN $TESTCASEDIR/$tfileName >&_tmp.txt
    # fix bison version diff
    awk -F : '{print $3}' $REFOUTDIR/${tfileName%.*}.out >_ref.txt

    #special case, mac's grep is stupid =.=..
    if [[ $tfileName == "test23.tig" ]]; then
      rm -f __ref.txt
      if [[ $SYS != "Linux" ]]; then
        cat _ref.txt >>__ref.txt
        cat _ref.txt >>__ref.txt
        rm _ref.txt
        mv __ref.txt _ref.txt
      fi
    fi
    #grep -Fof _ref.txt _tmp.txt >&__tmp.txt
	awk -F : '{print $3}' _tmp.txt >__tmp.txt

    diff $DIFFOPTION __tmp.txt _ref.txt >&.tmp.txt
    if [ -s .tmp.txt ]; then
      #cat .tmp.txt
      echo -e "${RED_COLOR}Your output:${RES}"
      cat _tmp.txt
      echo -e "${RED_COLOR}Expected output:${RES}"
      cat _ref.txt
      echo -e "${BLUE_COLOR}[*_*]$ite: Output Mismatch [$tfileName]${RES}"
      rm -f _tmp.txt _ref.txt .tmp.txt __tmp.txt
      exit 234
    fi
  fi
done
rm -f testcases refs _tmp.txt _ref.txt .tmp.txt __tmp.txt
echo -e "${GREEN_COLOR}[^_^]$ite: Pass Lab4. Score: 100.${RES}"
