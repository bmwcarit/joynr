#!/bin/bash

for app in `asadmin list-applications | grep 'ejb' | cut -d" " -f1`;
do
    echo "undeploy $app";
    asadmin undeploy --droptables=true $app;
done

asadmin stop-domain --kill=true
asadmin stop-database
