#!/bin/bash

# Give the JEE Discovery Directory a chance to start ...
sleep 30

asadmin --user admin --passwordFile=/opt/payara/passwordFile --interactive=false start-domain --debug --verbose &
PID=$!
sleep 60
asadmin --interactive=false --user admin --passwordFile=/opt/payara/passwordFile deploy /sit-jee-app.war
wait $PID
