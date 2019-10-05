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

BIN=tiger-compiler
#PROJDIR=lab6
TESTCASEDIR=testcases
RUNTIMEPATH=../src/tiger/runtime/runtime.c
MERGECASEDIR=testcases/merge
REFOUTDIR=refs
MERGEREFDIR=refs/merge
DIFFOPTION="-w -B"
score=0

base_name=$(basename "$PWD")
if [[ ! $base_name =~ "tiger-compiler" ]]; then
    echo "[-_-]: Not in Lab Root Dir"
    echo "SCORE: 0"
    exit 1
fi

mkdir -p build
cd build
rm -f testcases refs _tmp.txt .tmp.txt __tmp.txt _ref.txt
ln -s ../testdata/lab6/testcases testcases
if [[ $? != 0 ]]; then
    echo "[-_-]$ite: Link Error"
    echo "TOTAL SCORE: 0"
    exit 123
fi

ln -s ../testdata/lab6/refs refs
if [[ $? != 0 ]]; then
    echo "[-_-]$ite: Link Error"
    echo "TOTAL SCORE: 0"
    exit 123
fi

cmake .. >&/dev/null
make ${BIN} -j >/dev/null

#echo $?
if [[ $? != 0 ]]; then
    echo -e "${RED_COLOR}[-_-]$ite: Compile Error${RES}"
    make clean >&/dev/null
    exit 123
fi
for tcase in $(ls $TESTCASEDIR/); do
    if [ ${tcase##*.} = "tig" ]; then
        tfileName=${tcase##*/}
        ./$BIN $TESTCASEDIR/$tfileName &>/dev/null
        gcc -Wl,--wrap,getchar -m64 $TESTCASEDIR/${tfileName}.s $RUNTIMEPATH -o test.out &>/dev/null
        if [ ! -s test.out ]; then
            echo -e "${BLUE_COLOR}[*_*]$ite: Link error. [$tfileName]${RES}"
            rm $TESTCASEDIR/${tfileName}.s

            continue
            #exit 345
        fi

        if [ $tfileName = "merge.tig" ]; then
            result=0
            count=1
            for mergecase in $(ls $MERGECASEDIR); do
                ./test.out <$MERGECASEDIR/$mergecase >&_tmp.txt
                diff $DIFFOPTION _tmp.txt $MERGEREFDIR/${mergecase%.*}.out >&_ref.txt
                if [ -s _ref.txt ]; then
                    echo -e "${BLUE_COLOR}[*_*]$ite: Output mismatches. [$tfileName]${RES}"
                    rm -f _tmp.txt _ref.txt $TESTCASEDIR/${tfileName}.s test.out
                    continue
                    #exit 234
                fi
                result=$((result + 2))
                count=$((count + 1))
                echo "pass Merge"
                rm -f _tmp.txt _ref.txt
            done
            score=$((score + result + count / 2))
            rm -f test.out $TESTCASEDIR/${tfileName}.s
        else
            ./test.out >&_tmp.txt
            diff $DIFFOPTION _tmp.txt $REFOUTDIR/${tfileName%.*}.out >&_ref.txt
            if [ -s _ref.txt ]; then
                echo -e "${BLUE_COLOR}[*_*]$ite: Output mismatches. [$tfileName]${RES}"
                rm -f _tmp.txt _ref.txt $TESTCASEDIR/${tfileName}.s test.out
                continue
                #					exit 234
            fi

            rm -f _tmp.txt _ref.txt $TESTCASEDIR/${tfileName}.s test.out
            echo -e "pass ${tfileName}"
            tname=${tfileName##t}
            if [ $tname = $tfileName ]; then
                score=$((score + 10))
            else
                score=$((score + 5))
            fi

        fi
    fi
done

rm -f testcases refs _tmp.txt .tmp.txt
echo -e "${GREEN_COLOR}${ite}Your score: ${score}${RES}"
