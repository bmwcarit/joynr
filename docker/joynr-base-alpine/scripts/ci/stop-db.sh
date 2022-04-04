#!/bin/sh

# stop the postgresql db
echo "stop the postgresql db"
gosu postgres /usr/bin/pg_ctl -D /var/lib/postgresql/data -l /var/lib/postgresql/data/logfile stop
