#!/bin/bash

echo "### start running the system-integration-test ###"

echo "killing potentially existing containers, e.g. due to a previous failing build"
docker-compose stop
docker-compose rm -f

mkdir -p sit_onboard_logs

echo "starting the orchestra"
docker-compose up -d
if [ $? -ne 0 ]
then
  echo "ERROR: failed to start containers"
  exit 1
fi

echo "Wait 20 minutes, then log the result of the docker containers"
echo "Started: $(date)"
sleep 1200

echo "Logging results. Started: $(date). Timeout = 1000s"

# timeout runs the docker-compose logs command for 1000s, and if it is not terminated,
# it will kill it after ten seconds
timeout -k 10s 1000s docker-compose logs --no-color -t > sit-apps.log

echo "Logged: $(du -sh sit-apps.log)"

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
  echo "ERROR: no result found"
  exit 1
fi
failing=`cat sit-result.log | grep -v success | wc -l`
echo number of tests: `cat sit-result.log | wc -l`
echo      successful: `cat sit-result.log | grep success | wc -l`
echo         failing: $failing

#rm sit-apps.log
#rm sit-result.log
EXPECTED_RESULTS=79
if [ $results -ne $EXPECTED_RESULTS ]
then
  echo "ERROR: unexpected number of results: $results, expected $EXPECTED_RESULTS"
  exit 1
fi

echo "SIT SUCCESS (results: $results, expected: $EXPECTED_RESULTS)"

echo "### end running the system-integration-test ###"

exit $failing
