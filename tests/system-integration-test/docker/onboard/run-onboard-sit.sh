#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2016 BMW Car IT GmbH
# %%
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# #L%
###

echo "Onboard RUN END TO END TEST"

echo "ENVIRONMENT"
env

# fail on first error
set -e

DATA_DIR=/data
CPP_HOME=${DATA_DIR}/sit-cpp-app
NODE_HOME=${DATA_DIR}/sit-node-app
DOMAIN_PREFIX="io.joynr.systemintegrationtest"
CONSUMER_DOMAIN_PREFIXES=()
function print_usage {
	echo "Usage: ./run-onboard-sit.sh [--own-domain-prefix <own domain prefix>] [--consumer-domain-prefix <consumer domain prefix>]*

--own-domain-prefix: set the domain-prefix used by the c++ and node provider. This domain prefix is also used when
    setting up the consumer test
--consumer-domain-prefix: add a domain prefix used by the c++ and node consumers when looking for providers.
"
}

while [[ $# -gt 0 ]]
do
	key="$1"
	case $key in
		--own-domain-prefix)
		shift
		DOMAIN_PREFIX="$1"
		;;
		--consumer-domain-prefix)
		shift
		CONSUMER_DOMAIN_PREFIXES+=("$1")
		;;
		*)
		echo "Unknown argument: $1"
		print_usage
		exit -1
		;;
	esac
	shift
done

echo "adding $DOMAIN_PREFIX also to the list of consumer domain prefixes"
CONSUMER_DOMAIN_PREFIXES+=($DOMAIN_PREFIX)

echo "start cluster controller + providers with domain prefix $DOMAIN_PREFIX"
(
  cd ${CPP_HOME}/bin

  ./cluster-controller ${DATA_DIR}/onboard-cc-messaging.settings & CLUSTER_CONTROLLER_PID=$!

  ./jsit-provider-ws $DOMAIN_PREFIX.cpp runForever & CPP_PROVIDER_PID=$!

  cd ${NODE_HOME}

  npm run-script startprovider --sit-node-app:domain=$DOMAIN_PREFIX.node --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242 & NODE_PROVIDER_PID=$!
)

echo "Wait for JEE Application to be started"
(
	function wait_for_jee_endpoint {
		retry_count=0
		max_retries=60
		until curl -f -s http://sit-jee-app:8080/sit-jee-app/consumer/ping || ((retry_count++ > max_retries))
		do
			echo "JEE application not started yet ..."
			sleep 2
		done
		if (( retry_count > max_retries )); then
			echo "JEE application failed to start in time."
			exit -1
		fi
		return 0
	}
	wait_for_jee_endpoint && echo "JEE endpoint started, continuing"
)

for domainprefix in "${CONSUMER_DOMAIN_PREFIXES[@]}"
  do
  echo "run cpp consumer test"
  (
    cd ${CPP_HOME}/bin

    # cpp - run the test against cpp provider
    ./jsit-consumer-ws $domainprefix.cpp

    if [ "$?" == "-1" ]
    then
      echo "ERROR joynr cpp system integration test against cpp provider failed with error code $?"
    else
      echo "cpp<->cpp joynr system integration test succeeded"
    fi

    # cpp - run the test against node provider
    ./jsit-consumer-ws $domainprefix.node

    if [ "$?" == "-1" ]
    then
      echo "ERROR joynr cpp system integration test against node provider failed with error code $?"
    else
      echo "cpp<->node joynr system integration test succeeded"
    fi

    # cpp - run the test against jee provider
    ./jsit-consumer-ws $domainprefix.jee

    if [ "$?" == "-1" ]
    then
      echo "ERROR joynr cpp system integration test against jee provider failed with error code $?"
    else
      echo "cpp<->jee joynr system integration test succeeded"
    fi
  )

  echo "run node consumer test"
  (
    cd ${NODE_HOME}

    # node - run the test against cpp provider
    npm run-script startconsumer --sit-node-app:domain=$domainprefix.cpp --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242

    # node - run the test against node provider
    npm run-script startconsumer --sit-node-app:domain=$domainprefix.node --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242

    # node - run the test against jee provider
    npm run-script startconsumer --sit-node-app:domain=$domainprefix.jee --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242

  )
done

# Clean up
# disabled, as the provider and the cluster controller must still be available for the jee consumer test
# kill $CPP_PROVIDER_PID
# kill $NODE_PROVIDER_PID
# kill $CLUSTER_CONTROLLER_PID
# kill $MOSQUITTO_PID
wait
