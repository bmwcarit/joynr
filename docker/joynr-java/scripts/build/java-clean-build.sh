#!/bin/bash

# fail on first error
set -e

BASEDIR=$(readlink -f $(dirname ${BASH_SOURCE[0]}))

cd /data/src

# clean repository
mvn clean

# build joynr Code Generators and joynr Java
$BASEDIR/run-java-tests

