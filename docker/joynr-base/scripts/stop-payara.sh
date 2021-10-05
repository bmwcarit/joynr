#!/bin/bash

for app in `asadmin list-applications | grep 'ejb' | cut -d" " -f1`;
do
    echo "undeploy $app";
    asadmin --user admin undeploy --droptables=true $app;
done

asadmin --user admin stop-domain --kill=true
