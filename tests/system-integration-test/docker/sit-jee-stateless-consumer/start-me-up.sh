#!/bin/bash
sleep 30

asadmin --user admin --passwordFile=/opt/payara/passwordFile --interactive=false start-domain --debug --verbose &
PID=$!

# Give the JEE Discovery Directory a chance to start ...
# and wait grace period to let the server and database start up
sleep 30

asadmin --interactive=false --user admin --passwordFile=/opt/payara/passwordFile deploy /sit-jee-stateless-consumer.war
# Prevent script from exiting which would cause the container to terminate immediately
wait $PID
