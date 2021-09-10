#!/bin/sh

# stop the postgresql db
echo "stop the postgresql db"
gosu postgres /usr/bin/pg_ctl -D /var/lib/pgsql/data -l /var/lib/pgsql/data/logfile stop
