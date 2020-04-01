#!/bin/bash

docker_ps=$(docker ps -a)
conts=$(echo $docker_ps | cut -d' ' -f9-)
if [ ! -z "$conts" ]; then
    echo "Stopping running docker-compose containers, e.g. due to a previous failing build"
    docker-compose stop
    docker-compose rm -f
fi

dangling_images=$(docker images --filter="dangling=true" -q)
if [ ! -z "$dangling_images" ]; then
    echo "Remove potential dangling images from an old run of build_images"
    docker images --filter="dangling=true" -q | xargs docker rmi
fi

old_logs=$(find . -name "*.log")
if [ ! -z "$old_logs" ]; then
    echo "Cleaning old logs"
    rm *.log
fi

echo "Starting the orchestra"
docker-compose up -d

if [ $? -ne 0 ]
then
  echo "ERROR: failed to start containers"
  exit 1
fi

echo "Waiting 180 secs, then log the result of the docker-compose containers. More runs and services need more waiting time"

sleep 180

docker-compose logs --no-color > logsOfAllContainers.log

cat logsOfAllContainers.log

cat logsOfAllContainers.log | grep -a 'PT RESULT' > pt-result.log

echo "PT results:"
cat pt-result.log
results=`cat pt-result.log | wc -l`
if [ $results -eq 0 ]
then
  echo "ERROR: no result found"
  exit 1
fi
failing=`cat pt-result.log | grep -v success | wc -l`
echo number of tests: `cat pt-result.log | wc -l`
echo      successful: `cat pt-result.log | grep success | wc -l`
echo         failing: $failing

EXPECTED_RESULTS=1

echo "Stop all containers"
docker-compose stop

echo "Remove all containers"
docker-compose rm -f

if [ $results -ne $EXPECTED_RESULTS ]
then
  echo "ERROR: unexpected number of results: $results, expected $EXPECTED_RESULTS"
  exit 1
fi

exit $failing
