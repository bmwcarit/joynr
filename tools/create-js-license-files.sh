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
    echo "Use only with a clean git repository. Otherwise package.json files of installed"
    echo "depenendices will be evaluated as well."
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
        --setup )                shift
                                 SETUP=ON
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
    npm -g install licensecheck
fi

# List all available package.json files. Do this before the mvn install
# call in order to prevent enumerating the package.json files of the installed
# dependencies.
PACKAGE_JSON_DIRECTORIES=$(find $JOYNR_SOURCE_DIR -name package.json -printf '%h\n')

(
cd $JOYNR_SOURCE_DIR
mvn install -P javascript
)

for PACKAGE_JSON_DIRECTORY in $PACKAGE_JSON_DIRECTORIES
do
    (
        echo "### Installing in $PACKAGE_JSON_DIRECTORY ###"
        cd $PACKAGE_JSON_DIRECTORY
        npm install

        cat ${JOYNR_SOURCE_DIR}tools/NOTICE-JS-PREFIX.txt > ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS
        licensecheck --opt --dev >> ${PACKAGE_JSON_DIRECTORY}/NOTICE-JS

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
