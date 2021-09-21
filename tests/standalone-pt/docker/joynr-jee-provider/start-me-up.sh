#!/bin/bash

# we cannot use the --verbose anymore since this would block
#asadmin start-domain --verbose
asadmin --user admin --passwordfile=/opt/payara/passwordFile --interactive=false start-domain --debug --verbose &
PID=$!
# sleep a few seconds to give the broker to bootup
sleep 40
asadmin --user admin --passwordfile=/opt/payara/passwordFile deploy /discovery-directory-pt-jee.war

# should we have a tail -f here?
# where are the logs in this payara installation?
wait $PID

echo "end of start-me-up of joynr-jee-provider"
