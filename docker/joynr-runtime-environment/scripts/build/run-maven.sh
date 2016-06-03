#!/bin/bash

function usage
{
    echo "usage: run-maven.sh"
}

while [ "$1" != "" ]; do
echo "PARAM is: $1"
    case $1 in
        -h | --help )            usage
                                 exit
                                 ;;
        * )                      usage
                                 exit 1
    esac
    shift
done

mvn clean install -P javascript
(cd tests/test-base && mvn install)
(cd tests/performance-test && mvn install -P create-standalone-jars)
