#!/bin/bash

function wait_for_endpoint {
        retry_count=0
        max_retries=60
        until curl -f -s $1 || ((retry_count++ > max_retries))
        do
                echo "$1 not available yet ..."
                sleep 5
        done
        if (( retry_count > max_retries )); then
                echo "$1 not reachable in time."
                return -1
        fi
        return 0
}

echo "killing potentially existing containers"
docker compose stop
docker compose rm -f

echo -e "\nstarting the orchestra"
docker compose up -d
if [ $? -ne 0 ]
then
  echo "failed to start containers"
  exit 1
fi

echo -e "\nwait for containers to start up and for joynr provider to become available"
sleep 40
wait_for_endpoint "http://localhost:8080/control/ping"

echo -e "\n\ntrigger a joynr method call on the consumer"
curl http://localhost:8081/control/trigger
sleep 5

docker compose logs -t > custom-headers-full.log

echo -e "\n\nstop all containers"
docker compose stop

echo -e "\nremove all containers"
docker compose rm -f
