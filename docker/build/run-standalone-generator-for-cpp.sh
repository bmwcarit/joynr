#!/bin/bash

GENERATOR=$(find /src/tools/generator/joynr-generator-standalone/target -name 'joynr-generator-standalone-*.jar' | grep -v sources)

java -jar $GENERATOR \
     -modelpath /src/basemodel/src/main/franca \
     -outputPath /src/cpp/libjoynr/basemodel/generated \
     -generationLanguage cpp
