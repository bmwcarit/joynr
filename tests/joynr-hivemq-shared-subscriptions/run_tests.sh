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

echo "wait 6 minutes, then log the result of the docker containers"
sleep 360

docker-compose logs -t > sst-full.log

echo "stop all containers"
docker-compose stop

echo "remove all containers"
docker-compose rm -f
