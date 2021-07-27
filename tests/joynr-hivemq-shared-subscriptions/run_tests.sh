#!/bin/bash

echo "killing potentially existing containers, e.g. due to a previous failing build"
docker-compose stop
docker-compose rm -f

echo "starting the orchestra"
docker-compose up -d
if [ $? -ne 0 ]
then
  echo "failed to start containers"
  exit 1
fi

echo "Wait 6 minutes, then log the result of the docker containers"
echo "Started: $(date)"
sleep 360

echo "Logging results. Started: $(date). Timeout = 240s"

# timeout runs the docker-compose logs command for 240s, and if it is not terminated,
# it will kill it after ten seconds
timeout -k 10s 240s docker-compose logs --no-color -t > sst-full.log

echo "Logged: $(du -sh sst-full.log)"

echo "stop all containers"
docker-compose stop

echo "remove all containers"
docker-compose rm -f
