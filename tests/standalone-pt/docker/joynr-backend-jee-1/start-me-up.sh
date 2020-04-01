#!/bin/bash

# we cannot use the --verbose anymore since this would block
#asadmin start-domain --verbose
asadmin --interactive=false start-domain --debug --verbose &
PID=$!
# sleep a few seconds to give the broker time to bootup
sleep 20
asadmin --user admin --passwordfile=/opt/payara41/pwdfile deploy /discovery-directory-jee-shared-db.war
asadmin --user admin --passwordfile=/opt/payara41/pwdfile deploy /domain-access-controller-jee.war

# should we have a tail -f here?
# where are the logs in this payara installation?
wait $PID

echo "end of start-me-up of joynr-backend-jee-1"
