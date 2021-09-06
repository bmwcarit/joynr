#!/bin/bash

set -xe

/usr/bin/initdb --username=gcd --pwfile=/opt/gcd_password -D /var/lib/pgsql/data
echo "listen_addresses = '*'" >> /var/lib/pgsql/data/postgresql.conf
echo "host all all all md5" >> /var/lib/pgsql/data/pg_hba.conf

/usr/bin/pg_ctl -D /var/lib/pgsql/data -l /var/lib/pgsql/data/logfile start

/usr/bin/psql -v ON_ERROR_STOP=1 --username gcd --no-password --dbname postgres --set db="gcd" <<-'EOSQL'
        CREATE DATABASE :"db"
EOSQL
echo

/usr/bin/psql -f /data/scripts/init.sql -v ON_ERROR_STOP=1 --username gcd --no-password
/usr/bin/pg_ctl -D /var/lib/pgsql/data -l /var/lib/pgsql/data/logfile stop
