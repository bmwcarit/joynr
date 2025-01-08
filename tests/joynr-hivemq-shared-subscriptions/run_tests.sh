#!/bin/bash

# build joynr backend images
cd ../../docker/ && ./build_backend.sh
cd -

echo "killing potentially existing containers, e.g. due to a previous failing build"
docker compose stop
docker compose rm -f

echo "starting the orchestra"
docker compose up -d
if [ $? -ne 0 ]
then
  echo "failed to start containers"
  exit 1
fi

echo "Wait 6 minutes, then log the result of the docker containers"
echo "Started: $(date)"
sleep 360

echo "Logging results. Started: $(date). Timeout = 240s"

# timeout runs the docker compose logs command for 240s, and if it is not terminated,
# it will kill it after ten seconds
timeout -k 10s 240s docker compose logs --no-color -t > sst-full.log

echo "Logged: $(du -sh sst-full.log)"

echo "stop all containers"
docker compose stop

echo "remove all containers"
docker compose rm -f

# Check SST test result
if grep -F -q '[SST] Triggered 100 pings. 100 were successful' sst-full.log ; then
    echo "[SharedSubsTest] SUCCESS" >> sst-full.log
else
    echo "[SharedSubsTest] FAILURE" >> sst-full.log
fi

# Check backpressure test result
node1_bp_enter_count=$(grep -c 'backpressure-provider-1.*Backpressure mode entered' sst-full.log)
node1_bp_exit_count=$(grep -c 'backpressure-provider-1.*Backpressure mode exited' sst-full.log)
node2_bp_enter_count=$(grep -c 'backpressure-provider-2.*Backpressure mode entered' sst-full.log)
node2_bp_exit_count=$(grep -c 'backpressure-provider-2.*Backpressure mode exited' sst-full.log)

if grep -F -q '[BPT] Triggered 1000 pings. 1000 were successful' sst-full.log && \
[ ${node1_bp_enter_count} -gt 0 ] && [ ${node1_bp_enter_count} -eq ${node1_bp_exit_count} ] && \
[ ${node2_bp_enter_count} -gt 0 ] && [ ${node2_bp_enter_count} -eq ${node2_bp_exit_count} ]; then
    echo "[BackpressureTest] SUCCESS" >> sst-full.log
else
    echo "[BackpressureTest] FAILURE" >> sst-full.log
fi
