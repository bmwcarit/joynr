#!/bin/bash

###
# #%L
# %%
# Copyright (C) 2011 - 2017 BMW Car IT GmbH
# %%
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# #L%
###
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
