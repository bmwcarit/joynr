#!/bin/bash

# we cannot use the --verbose anymore since this would block
#asadmin start-domain --verbose
asadmin --interactive=false start-domain --debug --verbose &
PID=$!
# sleep a few seconds to give the broker to bootup
sleep 20
asadmin --user admin --passwordfile=/opt/payara41/pwdfile deploy /discovery-directory-pt-jee-shared-db.war

# should we have a tail -f here?
# where are the logs in this payara installation?
wait $PID

echo "end of start-me-up of joynr-faked-backend-jee-1"