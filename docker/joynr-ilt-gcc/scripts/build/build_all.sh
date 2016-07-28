#!/bin/bash

./cpp-install-muesli.sh --default
./run-java-formatter
./java-clean-build skipTests
./run-javascript-formatter
./javascript-clean-build skipTests
./cpp-generate.sh
./cpp-clean-build.sh --buildtests OFF --enableclangformatter OFF
./cpp-build-tests.sh all
