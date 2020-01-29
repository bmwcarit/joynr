#!/bin/bash
set -e

JOYNR_SOURCE_DIR=
SETUP=OFF
VERSION=

os=`uname`

function usage
{
    echo "usage: create-js-license-files.sh --joynrsourcedir <sourcedir> --version <joynrversion> [--setup]"
    echo "Use the --setup option to install the licensecheck tool. Sudo may be required"
    echo "joynrversion must match the current version. For example: 0.00.0-SNAPSHOT"
}

function _sed
{
    if [[ "$os" =~ "Linux" ]]; then
        sed -i -e "$1" ${@:2}
    else
        sed -i '' -e "$1" ${@:2}
    fi
}

function removeVersion
{
    DEPENDENCYNAME=$1
    FILENAME=$2

    _sed "s/${DEPENDENCYNAME} (${VERSION})/${DEPENDENCYNAME}/g" ${FILENAME}
}

while [ "$1" != "" ]; do
    case $1 in
        --joynrsourcedir )       shift
                                 JOYNR_SOURCE_DIR=${1%/}/
                                 ;;
        --version )              shift
                                 VERSION=$1
                                 ;;
        --setup )                SETUP=ON
                                 ;;
        * )                      usage
                                 exit 1
    esac
    shift
done

if [ "$JOYNR_SOURCE_DIR" == "" ] || [ "$VERSION" == "" ]
then
    usage
    exit 1
fi

if [ "$SETUP" == "ON" ]
then
    echo "Install licensecheck:"
    npm -g install licensecheck
fi

# List all available package.json files.
# Update this list with the following command if new package.json are included
# the repository.
#PACKAGE_JSON_DIRECTORIES=$(find $JOYNR_SOURCE_DIR -name package.json -printf '%h\n')
declare -a PACKAGE_JSON_DIRECTORIES=(
  "$JOYNR_SOURCE_DIR/tests/robustness-test"
  "$JOYNR_SOURCE_DIR/tests/performance-test"
  "$JOYNR_SOURCE_DIR/tests/test-base"
  "$JOYNR_SOURCE_DIR/tests/system-integration-test/sit-node-app"
  "$JOYNR_SOURCE_DIR/examples/radio-node"
  "$JOYNR_SOURCE_DIR/javascript/libjoynr-js/src/main/js"
  "$JOYNR_SOURCE_DIR/javascript/libjoynr-js/joynr-generator-npm"
  "$JOYNR_SOURCE_DIR/javascript/libjoynr-js/joynr-generator-npm-test"
  #"$JOYNR_SOURCE_DIR/tests/inter-language-test"
  #"$JOYNR_SOURCE_DIR/javascript/libjoynr-js/src/test/resources/node/shutdown"
  #"$JOYNR_SOURCE_DIR/javascript/libjoynr-js/src/main/browserify"
  #"$JOYNR_SOURCE_DIR/javascript/libjoynr-js"
)

(
cd $JOYNR_SOURCE_DIR
mvn install -P javascript
)

for PACKAGE_JSON_DIRECTORY in "${PACKAGE_JSON_DIRECTORIES[@]}"
do
    (
        echo "### Installing in $PACKAGE_JSON_DIRECTORY ###"
        cd $PACKAGE_JSON_DIRECTORY
        npm install

        cat ${JOYNR_SOURCE_DIR}tools/NOTICE-JS-HEADER.txt > ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        licensecheck >> ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS

        removeVersion "joynr" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        removeVersion "radio-node" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        removeVersion "inter-language-test" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        removeVersion "performance-test" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        removeVersion "robustness-test" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        removeVersion "sit-node-app" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        removeVersion "test-base" ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS

        if [ -f ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS-manual ]
        then
            # Add a separator
            echo "" >> ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
            cat ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS-manual >> ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        fi
    )
done
