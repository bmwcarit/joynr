#!/bin/bash

export PATH=$PATH:/opt/payara41/glassfish/bin
asadmin --interactive=false start-domain --debug --verbose &
PID=$!
# Wait grace period to let the server and database start up
sleep 20
asadmin --interactive=false --user admin --passwordfile=/tmp/pwdfile deploy /backpressure-monitor-app.war
# Prevent script from exiting which would cause the container to terminate immediately
wait $PID
