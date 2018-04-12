#!/bin/bash
./java-clean-build skipTests

#javascript tests are needed for fixRequire and gyp rebuild
./javascript-clean-build
./cpp-clean-build.sh --buildtests OFF --enableclangformatter OFF --buildtype RELEASE --additionalcmakeargs "-DUSE_PLATFORM_MUESLI=OFF"

./run-js-performance-tests.sh
