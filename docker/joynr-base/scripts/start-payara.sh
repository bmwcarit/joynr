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

### configure log levels for debugging here, if required
# asadmin set-log-levels io.joynr.messaging=FINEST
### hide periodic RoutingTable.purge logs
# asadmin set-log-levels io.joynr.messaging.routing.RoutingTableImpl=FINE

if [ -d "/data/logs" ]
then
    asadmin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.file=/data/logs/payara.log
fi
asadmin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.logtoConsole=true
asadmin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.multiLineMode=false
asadmin set-log-attributes com.sun.enterprise.server.logging.GFFileHandler.rotationLimitInBytes=512000000

asadmin stop-domain
asadmin start-domain --verbose &
sleep 20
echo '####################################################'
echo '# PAYARA list-log-levels'
echo '####################################################'
asadmin list-log-levels

for warFile in $(echo $warFileList | tr "," "\n")
do
    asadmin deploy --force=true $warFile
done

