#!/bin/bash

GENERATOR=$(find /src/tools/generator/joynr-generator-standalone/target -name 'joynr-generator-standalone-*.jar' | grep -v sources)

java -jar $GENERATOR \
     -modelpath /src/basemodel/src/main/franca \
     -outputPath /src/cpp/libjoynr/basemodel/generated \
     -generationLanguage cpp

java -jar $GENERATOR \
     -modelpath /src/basemodel/src/test/franca \
     -outputPath /src/cpp/tests/gen \
     -generationLanguage cpp

java -jar $GENERATOR \
     -modelpath /src/basemodel/src/test/franca-with-version \
     -outputPath /src/cpp/tests/gen-with-version \
     -generationLanguage cpp \
     -addVersionTo comment

java -jar $GENERATOR \
     -modelpath /src/basemodel/src/test/franca-unversioned \
     -outputPath /src/cpp/tests/gen-with-version \
     -generationLanguage cpp \
     -addVersionTo comment
