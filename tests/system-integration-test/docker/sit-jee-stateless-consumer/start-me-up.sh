#!/bin/bash

asadmin --interactive=false start-domain --debug --verbose &
PID=$!

# Give the JEE Discovery Directory a chance to start ...
# and wait grace period to let the server and database start up
sleep 30

asadmin --interactive=false --user admin --passwordfile=/tmp/pwdfile deploy /sit-jee-stateless-consumer.war
# Prevent script from exiting which would cause the container to terminate immediately
wait $PID
