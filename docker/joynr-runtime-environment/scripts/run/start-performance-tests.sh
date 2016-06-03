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

data/src/tests/performance-test/run-performance-tests.sh -j /usr/local/jetty -p /usr/bin -r /tmp -s /opt/joynr/performance-test/ -t CPP_SYNC -y /usr/bin -c 2 -x 1 -m OFF
