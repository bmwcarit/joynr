#!/bin/bash

set -xe

/usr/bin/initdb --username=gcd --pwfile=/opt/gcd_password -D /var/lib/postgresql/data
echo "listen_addresses = '*'" >> /var/lib/postgresql/data/postgresql.conf
echo "host all all all md5" >> /var/lib/postgresql/data/pg_hba.conf

/usr/bin/pg_ctl -D /var/lib/postgresql/data -l /var/lib/postgresql/data/logfile start

/usr/bin/psql -v ON_ERROR_STOP=1 --username gcd --no-password --dbname postgres --set db="gcd" <<-'EOSQL'
        CREATE DATABASE :"db"
EOSQL
echo

/usr/bin/psql -f /data/scripts/init.sql -v ON_ERROR_STOP=1 --username gcd --no-password
/usr/bin/pg_ctl -D /var/lib/postgresql/data -l /var/lib/postgresql/data/logfile stop
