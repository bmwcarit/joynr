#/bin/bash

source /data/src/docker/joynr-base/scripts/ci/start-and-stop-gcd-service.sh
cd /data/src

# fail on first error
# exit immediately if a command exits with a non-zero status
set -e

/data/src/docker/joynr-base/scripts/ci/start-db.sh

cp /data/src/docker/joynr-base/mosquitto.conf /home/joynr/mosquitto.conf
if [ -d "/data/logs" ]
then
    echo "log_dest file /data/logs/mosquitto.log" >> /home/joynr/mosquitto.conf
fi
mosquitto -c /home/joynr/mosquitto.conf &
MOSQUITTO_PID=$!

function stopservices
{
    stopGcd

    echo "stop mosquitto"
    kill -TERM $MOSQUITTO_PID
    wait $MOSQUITTO_PID
    /data/src/docker/joynr-base/scripts/ci/stop-db.sh
}

function stopdomain
{
    echo "undeploy and stop domain"
    asadmin undeploy radio-jee-provider
    asadmin undeploy radio-jee-consumer
    asadmin stop-domain
}

set +e # stop services even if there are failing tests or startGcd fails
startGcd
SUCCESS=$?
if [ "$SUCCESS" != "0" ]; then
    echo '########################################################'
    echo '# Start GCD failed with exit code:' $SUCCESS
    echo '########################################################'

    stopservices
    exit $SUCCESS
fi

echo '####################################################'
echo '# Test deploying war'
echo '####################################################'

asadmin start-domain
if [ $? != 0 ] ; then
	echo 'Error with start-domain';
	stopdomain
	stopservices
	exit 1;
fi

asadmin deploy examples/radio-jee/radio-jee-provider/target/radio-jee-provider.war
if [ $? != 0 ] ; then
	echo "deply radio-jee-provider.war failed";
	stopdomain
	stopservices
	exit 1;
fi

asadmin deploy examples/radio-jee/radio-jee-consumer/target/radio-jee-consumer.war
if [ "$deploy_consumer" == "Command deploy failed." ] ; then
	echo "deply radio-jee-consumer.war failed";
	stopdomain
	stopservices
	exit 1;
fi

echo -e "\n"
echo '####################################################'
echo '# Test endpoints'
echo '####################################################'

STATUS=$(curl -f -s -X POST -k --header "Content-Type:text/plain" --data 'StationOneTest' http://localhost:8080/radio-jee-consumer/radio-stations)
if [ $? -eq 22 ] ; then
	echo 'Error add radio-station';
	echo "$STATUS";
	echo -e "\n";
	stopdomain
	stopservices
	exit 1;
else
	echo -e "\n"
	echo "/radio-station :   OK"
fi

STATUS=$(curl -f -s -X POST http://localhost:8080/radio-jee-consumer/radio-stations/shuffle)
if [ $? -eq 22 ] ; then
	echo 'Error shuffle';
	echo "$STATUS";
	echo -e "\n";
	stopdomain
	stopservices
	exit 1;
else
	echo "/shuffle :         OK"
fi
	
STATUS=$(curl -f -s http://localhost:8080/radio-jee-consumer/radio-stations/current-station)
if [ $? -eq 22 ] ; then
	echo 'Error current-station';
	echo "$STATUS";
	echo -e "\n";
	stopdomain
	stopservices
	exit 1;
else
	echo "/current-station : OK"
fi
	
STATUS=$(curl -f -s http://localhost:8080/radio-jee-provider/control/metrics)
if [ $? -eq 22 ] ; then
	echo 'Error metrics';
	echo "$STATUS";
	echo -e "\n";
	stopdomain
	stopservices
	exit 1;
else
	echo "/metrics :         OK"
fi

echo '####################################################'
echo '# Test endpoints done'
echo '####################################################'
echo -e "\n"

echo '####################################################'
echo '# Test addFavoriteStation'
echo '####################################################'


addFavorite_status=$(curl -X POST --header "Content-Type:text/plain" --data 'StationOne' http://localhost:8080/radio-jee-consumer/radio-stations/)

if [ $(echo "$addFavorite_status" | grep -qi "ERROR") ] ; then
    echo "Failed, check logs:" 
    echo "$addFavorite_status"
fi

echo '####################################################'
echo '# Test addFavoriteStation duplicate'
echo '####################################################'


addFavorite_status=$(curl -X POST --header "Content-Type:text/plain" --data 'StationOne' http://localhost:8080/radio-jee-consumer/radio-stations/)

if [ $(echo "$addFavorite_status" | grep -qi "DUPLICATE") ] ; then
    echo "Failed, check logs:" 
    echo "$addFavorite_status"
fi

echo -e "\n"
echo '####################################################'
echo '# Test shuffle-station'
echo '####################################################'

shuffle_status=$(curl -X POST http://localhost:8080/radio-jee-consumer/radio-stations/shuffle)

if [ $(echo "$shuffle_status" | grep -qi "ERROR") ] ; then
    echo "Failed, check logs:" 
    echo "$addFavorite_status"
fi

echo -e "\n"
echo '####################################################'
echo '# Test current-station'
echo '####################################################'
current_status=$(curl http://localhost:8080/radio-jee-consumer/radio-stations/current-station)

if [ $(echo "$current_status" | grep -qi "ERROR") ] ; then
    echo "Failed, check logs:" 
    echo "$addFavorite_status"
fi

echo -e "\n"
echo '####################################################'
echo '# Test metrics'
echo '####################################################'
metrics_status=$(curl http://localhost:8080/radio-jee-provider/control/metrics)

if [ $(echo "$metrics_status" | grep -qi "ERROR") ] ; then
    echo "Failed, check logs:" 
    echo "$addFavorite_status"
fi

echo -e "\n"
echo '####################################################'
echo '# stop services'
echo '####################################################'

stopdomain
stopservices
