#!/bin/sh

# start the postgresql db
echo "start the postgresql db"
gosu postgres /usr/bin/pg_ctl -D /var/lib/postgresql/data -l /var/lib/postgresql/data/logfile start
