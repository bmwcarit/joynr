#!/bin/bash

# Give the JEE Discovery Directory a chance to start ...
sleep 30

asadmin --interactive=false start-domain --debug --verbose &
PID=$!
sleep 30
asadmin --interactive=false --user admin --passwordfile=/opt/payara41/pwdfile deploy /sit-jee-app.war
wait $PID
