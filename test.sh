#!/bin/bash
START_DIR=$(pwd)
SOURCE_DIR=$1
TESTS_DIR="test_cases"

echo "Source Dir: ""$SOURCE_DIR"
cd "$SOURCE_DIR"

if [ -d $TESTS_DIR ]; then
	echo "Test directory already exits"
else
	git clone https://github.com/hasherezade/bearparser_tests.git
	mv bearparser_tests $TESTS_DIR
fi

BCMD_DIR=$(pwd)/build/
FAILED=0

tests_list=$SOURCE_DIR/"tests_list.txt"

cd $TESTS_DIR
chmod +x test1.sh

while IFS=' ' read -r p1 p2
do
	if [[ "$p2" == "" || "$p1" == "" ]]; then
		continue;
	fi
	if [[ "$p1" == "#" ]]; then # allow for commenting out lines
		continue;
	fi

	./test1.sh "$BCMD_DIR" "$p2" "$p1"
	if [[ "$?" != 0 ]]; then
		FAILED=$FAILED+1
	fi
done < "$tests_list"

if [[ "$FAILED" == 0 ]]; then
	echo "All passed"
fi

cd "$START_DIR"

