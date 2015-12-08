#!/bin/bash

if (( $# != 2 )); then
    echo "Improper usage of this script. Please invoke with arguments <OLD_VERSION> <NEW_VERSION> "
    exit 1
fi

oldVersion=$1
newVersion=$2
newVersionFiltered=`echo $newVersion | sed -e "s/-SNAPSHOT//g"`
IFS='.' read -a version <<< "$newVersionFiltered"

echo sed -i '' 's/set(JOYNR_MAJOR_VERSION .*)/set(JOYNR_MAJOR_VERSION '${version[0]}')/g' cpp/CMakeLists.txt
sed -i '' 's/set(JOYNR_MAJOR_VERSION .*)/set(JOYNR_MAJOR_VERSION '${version[0]}')/g' cpp/CMakeLists.txt
echo sed -i '' 's/set(JOYNR_MINOR_VERSION .*)/set(JOYNR_MINOR_VERSION '${version[1]}')/g' cpp/CMakeLists.txt
sed -i '' 's/set(JOYNR_MINOR_VERSION .*)/set(JOYNR_MINOR_VERSION '${version[1]}')/g' cpp/CMakeLists.txt
echo sed -i '' 's/set(JOYNR_PATCH_VERSION .*)/set(JOYNR_PATCH_VERSION '${version[2]}')/g' cpp/CMakeLists.txt
sed -i '' 's/set(JOYNR_PATCH_VERSION .*)/set(JOYNR_PATCH_VERSION '${version[2]}')/g' cpp/CMakeLists.txt

echo sed -i '' 's/find_package(Joynr .*/find_package(Joynr '${newVersionFiltered}' REQUIRED)/g' examples/radio-app/CMakeLists.txt
sed -i '' 's/find_package(Joynr .*/find_package(Joynr '${newVersionFiltered}' REQUIRED)/g' examples/radio-app/CMakeLists.txt

echo mvn versions:set -o -P android,javascript -DnewVersion=$2
mvn versions:set -o -P android,javascript -DnewVersion=$2
echo mvn versions:commit -o -P android,javascript
mvn versions:commit -o -P android,javascript

sed -i '' 's/'$oldVersion'/'$newVersion'/g' \
cpp/CMakeLists.txt \
examples/radio-app/CMakeLists.txt \
android/robolectric-integration-tests/src/test/AndroidManifest.xml \
android/robolectric-unittests/src/main/AndroidManifest.xml \
examples/android-location-provider/AndroidManifest.xml \
examples/android-location-consumer/AndroidManifest.xml \
java/backend-services/discovery-directory-servlet/pom.xml \
java/backend-services/domain-access-controller-servlet/pom.xml \
javascript/apps/radio-node/package.json \
javascript/libjoynr-js/src/main/resources/package.json

countFoundOldVersions=$(grep -r ${oldVersion} * | grep -v ReleaseNotes | wc -l)
if (($countFoundOldVersions > 0)); then
    echo "WARNING: a grep over your workspace emphasised that the oldVersion is still present in some of your resources. Please check manually!"
    grep -r ${oldVersion} * | grep -v ReleaseNotes
else
    git add -A && git commit -m "[Release] set version to $newVersion"
fi