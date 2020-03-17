#!/bin/bash -x
if (( $# != 2 )); then
    echo "Improper usage of this script. Please invoke with arguments <OLD_VERSION> <NEW_VERSION> "
    exit 1
fi

os=`uname`

function _sed {
    if [[ "$os" =~ "Linux" ]]; then
        sed -i -e "$1" ${@:2}
    else
        sed -i '' -e "$1" ${@:2}
    fi
}

oldVersion=$1
newVersion=$2
oldVersionWithoutSuffix=`echo $oldVersion | sed -e "s/-.*//g"`
newVersionWithoutSuffix=`echo $newVersion | sed -e "s/-.*//g"`
IFS='.' read -a version <<< "$newVersionWithoutSuffix"

_sed 's/set(JOYNR_MAJOR_VERSION .*)/set(JOYNR_MAJOR_VERSION '${version[0]}')/g' cpp/CMakeLists.txt
_sed 's/set(JOYNR_MINOR_VERSION .*)/set(JOYNR_MINOR_VERSION '${version[1]}')/g' cpp/CMakeLists.txt
_sed 's/set(JOYNR_PATCH_VERSION .*)/set(JOYNR_PATCH_VERSION '${version[2]}')/g' cpp/CMakeLists.txt

_sed 's/find_package(Joynr .*/find_package(Joynr '${newVersionWithoutSuffix}' REQUIRED)/g' \
examples/radio-app/CMakeLists.txt \
tests/dummyKeychain/CMakeLists.txt \
tests/inter-language-test/CMakeLists.txt \
tests/performance-test/CMakeLists.txt \
tests/robustness-test/CMakeLists.txt \
tests/robustness-test-env/CMakeLists.txt \
tests/system-integration-test/sit-cpp-app/CMakeLists.txt

mvn versions:set -P android,javascript -DnewVersion=$2
mvn versions:commit -P android,javascript

_sed 's/'$oldVersion'/'$newVersion'/g' \
cpp/CMakeLists.txt \
package.json \
tests/inter-language-test/CMakeLists.txt \
tests/inter-language-test/package.json \
tests/performance-test/CMakeLists.txt \
tests/performance-test/package.json \
tests/robustness-test/CMakeLists.txt \
tests/robustness-test/package.json \
tests/system-integration-test/sit-cpp-app/CMakeLists.txt \
tests/system-integration-test/sit-node-app/package.json \
tests/test-base/package.json \
examples/radio-app/CMakeLists.txt \
examples/radio-node/pom.xml \
examples/radio-node/package.json \
javascript/libjoynr-js/package.json \
javascript/libjoynr-js/src/main/js/package.json \
javascript/joynr-generator/package.json \
tools/generator/joynr-generator-gradle-plugin/build.gradle \
tools/generator/joynr-generator-npm/joynr-generator-npm-test/package.json \
tools/generator/joynr-generator-npm/package.json \
android/libjoynr-android-websocket-runtime/build.gradle \
android/libjoynr-android-websocket-runtime/README.md \
examples/android/android-hello-world/android-hello-world-provider/build.gradle \
examples/android/android-hello-world/android-hello-world-consumer/build.gradle \
android/libjoynr-android-integration-tests/test-radio-provider/build.gradle \
android/libjoynr-android-integration-tests/test-radio-provider/app/build.gradle \
android/libjoynr-android-integration-tests/test-radio-consumer/build.gradle \
android/libjoynr-android-integration-tests/test-radio-consumer/app/build.gradle \
android/clustercontroller-android-standalone/app/build.gradle \
android/joynr-android-binder-runtime/build.gradle \
android/slf4j-android-bindings/build.gradle \
examples/android/android-hello-world-binder/android-hello-world-binder-provider/build.gradle \
examples/android/android-hello-world-binder/android-hello-world-binder-consumer/build.gradle \

_sed 's/clustercontroller-standalone-'${oldVersion}'-jar-with-dependencies.jar/clustercontroller-standalone-'${newVersion}'-jar-with-dependencies.jar/g' \
java/core/clustercontroller-standalone/README

_sed 's/Version:        '${oldVersionWithoutSuffix}'/Version:        '${newVersionWithoutSuffix}'/g' \
cpp/distribution/joynr.spec \
tests/system-integration-test/docker/onboard/joynr-without-test.spec

(
	_sed 's/		<joynr.version>'${oldVersion}'<\/joynr.version>/		<joynr.version>'${newVersion}'<\/joynr.version>/g' \
	examples/hello-world/pom.xml
	_sed 's/	<version>'${oldVersion}'<\/joynr.version>/	<version>'${newVersion}'<\/joynr.version>/g' \
	examples/hello-world/pom.xml
	cd examples/hello-world
	mvn versions:set -DnewVersion=${newVersion}
	mvn versions:commit
)

if [[ $newVersion != *"SNAPSHOT"* ]]; then
  echo "It looks like you are about to release a new joynr version: check release notes"
  releaseNotesOK=$(grep -cE "# joynr $newVersion$" wiki/ReleaseNotes.md)
  if (($releaseNotesOK = 0)); then
    echo "ERROR: Release notes do not contain any section for $newVersion. Fix release notes."
    exit -1
  fi
fi

echo "prepare git patch"
countFoundOldVersions=$(git grep -F ${oldVersion} * | grep -v ReleaseNotes | wc -l)
if (($countFoundOldVersions > 0)); then
    echo "WARNING: a grep over your workspace emphasised that the oldVersion is still present in some of your resources. Please check manually!"
    git grep -F ${oldVersion} * | grep -v ReleaseNotes
else
    git add -u && git commit -m "[Release] set version to $newVersion"
fi
