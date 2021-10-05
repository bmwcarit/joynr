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

asadmin --user admin start-domain

if [ -d "/data/logs" ]
then
    asadmin --user admin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.file=/data/logs/payara.log
fi
asadmin --user admin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.rotationLimitInBytes=512000000

echo '####################################################'
echo '# PAYARA list-log-levels'
echo '####################################################'
asadmin --user admin list-log-levels

for warFile in $(echo $warFileList | tr "," "\n")
do
    asadmin --user admin deploy --force=true $warFile
done

