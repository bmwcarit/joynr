#!/bin/bash
./java-clean-build skipTests
./javascript-clean-build
./cpp-install-muesli.sh --default
./cpp-clean-build.sh --buildtests OFF --enableclangformatter OFF
./cpp-build-tests.sh inter-language-test

