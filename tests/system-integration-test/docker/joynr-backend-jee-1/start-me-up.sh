#!/bin/bash

# we cannot use the --verbose anymore since this would block
#asadmin start-domain --verbose
asadmin --user admin --passwordfile=/opt/payara41/pwdfile  --interactive=false start-domain --debug --verbose &
PID=$!
sleep 20
asadmin --user admin --passwordfile=/opt/payara41/pwdfile deploy /discovery-directory-jee-shared-db.war

# should we have a tail -f here?
# where are the logs in this payara installation?
wait $PID
