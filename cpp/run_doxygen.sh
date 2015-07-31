#!/bin/bash
#
# Usage: $0 doxygen_command source-directory
#
echo "Running doxygen ..."
TEMPFILE=/tmp/doxygen.out.$$
if [ -z $1 ]
then
    echo "No doxygen-exeutable provided!"
    echo "Usage: $0 doxygen-executable source-directory"
    exit 1
fi
if [ -z $2 ]
then
    echo "No source directory provided!"
    echo "Usage: $0 doxygen-executable source-directory"
    exit 1
fi
if [ ! -x $1 ]
then
    echo "$1 does not exist or is not an executable."
    exit 1
fi
if [ ! -d $2 -o ! -x $2 ]
then
    echo "$2 does not exist, is not a directory, or lacks access permissions."
    exit 1
fi
cd $2
$1 Doxyfile > $TEMPFILE 2>&1
STATUS=$?
if [ $STATUS != 0 ]
then
    cat $TEMPFILE
    rm -f $TEMPFILE
    echo "Finished doxygen - with errors"
    exit 1
else
    CNT=`grep "warning:" $TEMPFILE | wc -l`
    if [ $CNT != 0 ]
    then
        cat $TEMPFILE
        rm -f $TEMPFILE
        echo "Finished doxygen - $CNT warning(s)"
        exit 1
    fi
fi

# ok
rm -f $TEMPFILE
echo "Finished doxygen - ok"
exit 0
