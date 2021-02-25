#!/bin/bash

# we cannot use the --verbose anymore since this would block
#asadmin start-domain --verbose
asadmin --interactive=false start-domain --debug --verbose &
PID=$!
sleep 30
asadmin --user admin --passwordfile=/opt/payara41/pwdfile deploy /discovery-directory-jee-shared-db.war

wait $PID
