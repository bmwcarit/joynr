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

echo "wait 3 minutes, then log the result of the docker containers"
sleep 180

docker-compose logs > sit-apps.log

echo "stop all containers"
docker-compose stop

echo "remove all containers"
docker-compose rm -f

cat sit-apps.log

cat sit-apps.log | grep -a 'SIT RESULT' > sit-result.log

echo SIT results:
cat sit-result.log
results=`cat sit-result.log | wc -l`
if [ $results -eq 0 ]
then
  echo "no result found"
  exit 1
fi
failing=`cat sit-result.log | grep -v success | wc -l`
echo number of tests: `cat sit-result.log | wc -l`
echo      successful: `cat sit-result.log | grep success | wc -l`
echo         failing: $failing

rm sit-apps.log
rm sit-result.log
if [ $results -ne 17 ]
then
  echo "results missing"
  exit 1
fi

exit $failing

