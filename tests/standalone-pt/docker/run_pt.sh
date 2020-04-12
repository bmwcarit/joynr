#!/bin/bash

# reset network pool before deploying
docker network prune -f

docker_ps=$(docker ps -a)
conts=$(echo $docker_ps | cut -d' ' -f9-)
if [ ! -z "$conts" ]; then
    echo "Stopping running docker-compose containers, e.g. due to a previous failing build"
    docker-compose stop
    docker-compose rm -f
    docker system prune -f
fi

dangling_images=$(docker images --filter="dangling=true" -q)
if [ ! -z "$dangling_images" ]; then
    echo "Remove potential dangling images"
    docker images --filter="dangling=true" -q | xargs docker rmi
fi

if [ -f "logsOfAllContainers.log" ]; then
    echo "There are some old logs. Archiving them first"
    tar cvf logsOfAllContainers.$(date "+%Y.%m.%d-%H.%M.%S").tar.bz2 logsOfAllContainers.log
fi

old_logs=$(find . -name "*.log")
if [ ! -z "$old_logs" ]; then
    echo "Cleaning old logs"
    rm *.log
fi

if [ -d results ]; then
    echo "Results folder already exists. Archiving it first then clean it up"
    tar cvf results.$(date "+%Y.%m.%d-%H.%M.%S").tar.bz2 results
    rm -fr results
fi

mkdir results

echo "Starting the orchestra"
docker-compose up -d --scale cppapp=20

if [ $? -ne 0 ]
then
  echo "ERROR: failed to start containers"
  exit 1
fi

echo "Started: $(date)"
echo "Waiting for ctrl-c then log results. More runs and services need more waiting time"

trap stop_me SIGINT

function stop_me {
  echo ""
  echo "Ctrl-c is sent. Logging containers to logsOfAllContainers.log ..."
  docker-compose logs --no-color > logsOfAllContainers.log

  cat logsOfAllContainers.log | grep -a 'PT RESULT' > pt-result.log

  echo "PT results:"
  #cat pt-result.log
  results=`cat pt-result.log | wc -l`
  if [ $results -eq 0 ]
  then
    echo "ERROR: no result found"
    exit 1
  fi

  echo "Stop and remove all containers"
  docker-compose stop

  echo number of tests: `cat pt-result.log | wc -l`
  echo      successful: `cat pt-result.log | grep success | wc -l`

  # clean up
  docker-compose rm -f
  docker system prune -f
  rm pt-result.log

if [ -d results ]; then
    echo "Collecting results"
    cat ./results/* > ./results/mergeResults.csv
    awk '!unique[$1$2$3$4$5$6$7$8$9]++' ./results/mergeResults.csv > ./results/resultsOfAllContainers.csv
    rm ./results/mergeResults.csv
fi

  exit 0
}

while true; do :; done
