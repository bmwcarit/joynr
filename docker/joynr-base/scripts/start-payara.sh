#!/bin/bash

function usage
{
    echo "usage: start-payara --wars|-w <comma-separated list of war files that shall be deployed>]"
}

warFileList=""

while [ "$1" != "" ]; do
echo "PARAM is: $1"
    case $1 in
        -w | --wars )   shift
                        warFileList="$1"
                        ;;
        * )             usage
                        exit 1
    esac
    shift
done

asadmin start-domain
asadmin start-database --jvmoptions="-Dderby.storage.useDefaultFilePermissions=true"

for warFile in $(echo $warFileList | tr "," "\n")
do
    asadmin deploy --force=true $warFile
done
