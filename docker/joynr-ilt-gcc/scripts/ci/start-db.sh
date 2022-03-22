#!/bin/sh

# start the postgresql db
echo "start the postgresql db"
gosu postgres /usr/bin/pg_ctl -D /var/lib/pgsql/data -l /var/lib/pgsql/data/logfile start
