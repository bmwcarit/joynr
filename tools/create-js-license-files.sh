#!/bin/bash
set -e

JOYNR_SOURCE_DIR=
SETUP=OFF

function usage
{
    echo "usage: create-js-license-files.sh --joynrsourcedir <sourcedir> [--setup]"
    echo "Use the --setup option to install the licensecheck tool. Sudo may be required"
    echo "Use only with a clean git repository. Otherwise package.json files of installed"
    echo "depenendices will be evaluated as well"
}

while [ "$1" != "" ]; do
    case $1 in
        --joynrsourcedir )       shift
                                 JOYNR_SOURCE_DIR=$1
                                 ;;
        --setup )                shift
                                 SETUP=ON
                                 ;;
    esac
    shift
done

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

        licensecheck --opt --dev > DEPENDENCYTREE
    )
done
