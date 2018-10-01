#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2017 BMW Car IT GmbH
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

# Give the JEE Discovery Directory a chance to start ...
sleep 30

echo "Onboard RUN END TO END TEST"

echo "ENVIRONMENT"
env

# fail on first error
set -e

DATA_DIR=/data
CPP_HOME=${DATA_DIR}/sit-cpp-app
NODE_APP_HOME=${DATA_DIR}/sit-node-app
# Do not use JAVA_HOME, which has special meaning
JAVA_APP_HOME=${DATA_DIR}/sit-java-app
DOMAIN_PREFIX="io.joynr.systemintegrationtest"
CONSUMER_DOMAIN_PREFIXES=()
print_usage() {
	echo "Usage: ./run-onboard-sit.sh [--own-domain-prefix <own domain prefix>] [--consumer-domain-prefix <consumer domain prefix>]*

--own-domain-prefix: set the domain-prefix used by the c++ and node provider. This domain prefix is also used when
    setting up the consumer test
--consumer-domain-prefix: add a domain prefix used by the c++ and node consumers when looking for providers.
"
}

while [ $# -gt 0 ]
do
	key="$1"
	case $key in
		--own-domain-prefix)
			shift
			DOMAIN_PREFIX="$1"
			;;
		--consumer-domain-prefix)
			shift
			CONSUMER_DOMAIN_PREFIXES=$CONSUMER_DOMAIN_PREFIXES"$1"
			;;
		*)
			echo "Unknown argument: $1"
			print_usage
			exit 1
			;;
	esac
	shift
done

echo "adding $DOMAIN_PREFIX also to the list of consumer domain prefixes"
CONSUMER_DOMAIN_PREFIXES=$CONSUMER_DOMAIN_PREFIXES$DOMAIN_PREFIX

echo "start cluster controller + providers with domain prefix $DOMAIN_PREFIX"
(
	cd ${CPP_HOME}/bin
	/usr/bin/cluster-controller ${DATA_DIR}/onboard-cc-messaging.settings & CLUSTER_CONTROLLER_PID=$!
	./jsit-provider-ws $DOMAIN_PREFIX.cpp --runForever true & CPP_PROVIDER_PID=$!

	cd ${NODE_APP_HOME}
	npm run-script startprovider --sit-node-app:domain=$DOMAIN_PREFIX.node & NODE_PROVIDER_PID=$!
	npm run-script startprovidertls --sit-node-app:domain=$DOMAIN_PREFIX.nodeTls & NODE_TLS_PROVIDER_PID=$!

	cd ${JAVA_APP_HOME}
	java -cp *.jar io.joynr.systemintegrationtest.ProviderApplication $DOMAIN_PREFIX.java runForever & JAVA_PROVIDER_PID=$!
)

echo "Wait for JEE Application to be started"
(
	wait_for_jee_endpoint() {
		retry_count=0
		max_retries=60
		while [ $retry_count -le $max_retries ]
		do
			curl -f -s http://sit-jee-app:8080/sit-jee-app/consumer/ping
			if [ "$?" = 0 ]
			then
				break
			fi
			echo "JEE application not started yet ..."
			sleep 2
			retry_count=$(($retry_count+1))
		done

		if [ $retry_count -gt $max_retries ]
		then
			echo "JEE application failed to start in time."
			exit 1
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
        echo "run cpp<->cpp joynr system integration test"
		./jsit-consumer-ws $domainprefix.cpp

		if [ "$?" = "0" ]
		then
			echo "cpp<->cpp joynr system integration test succeeded"
		else
			echo "ERROR joynr cpp system integration test against cpp provider failed with error code $?"
		fi

		# cpp - run the test against java provider
        echo "run cpp<->java joynr system integration test"
		./jsit-consumer-ws $domainprefix.java

		if [ "$?" = "0" ]
		then
			echo "cpp<->java joynr system integration test succeeded"
		else
			echo "ERROR joynr cpp system integration test against java provider failed with error code $?"
		fi

		# cpp - run the test against node provider
        echo "run cpp<->node joynr system integration test"
		./jsit-consumer-ws $domainprefix.node

		if [ "$?" = "0" ]
		then
			echo "cpp<->node joynr system integration test succeeded"
		else
			echo "ERROR joynr cpp system integration test against node provider failed with error code $?"
		fi

		# cpp - run the test against jee provider
        echo "run cpp<->jee joynr system integration test"
		./jsit-consumer-ws $domainprefix.jee

		if [ "$?" = "0" ]
		then
			echo "cpp<->jee joynr system integration test succeeded"
		else
			echo "ERROR joynr cpp system integration test against jee provider failed with error code $?"
		fi
	)

	echo "run java consumer test"
	(
		cd ${JAVA_APP_HOME}

		# java - run the test against cpp provider
        echo "run java<->cpp joynr system integration test"
		java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication $domainprefix.cpp

		if [ "$?" = "0" ]
		then
			echo "java<->cpp joynr system integration test succeeded"
		else
			echo "ERROR joynr java system integration test against cpp provider failed with error code $?"
		fi

		# java - run the test against java provider
        echo "run java<->java joynr system integration test"
		java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication $domainprefix.java

		if [ "$?" = "0" ]
		then
			echo "java<->java joynr system integration test succeeded"
		else
			echo "ERROR joynr java system integration test against java provider failed with error code $?"
		fi

		# java - run the test against node provider
        echo "run java<->node joynr system integration test"
		java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication $domainprefix.node

		if [ "$?" = "0" ]
		then
			echo "java<->node joynr system integration test succeeded"
		else
			echo "ERROR joynr java system integration test against node provider failed with error code $?"
		fi

		# java - run the test against jee provider
        echo "run java<->jee joynr system integration test"
		java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication $domainprefix.jee

		if [ "$?" = "0" ]
		then
			echo "java<->jee joynr system integration test succeeded"
		else
			echo "ERROR joynr java system integration test against jee provider failed with error code $?"
		fi
	)

	echo "run node consumer test"
	(
		cd ${NODE_APP_HOME}

		# node - run the test against cpp provider
        echo "run node<->cpp joynr system integration test"
		npm run-script startconsumer --sit-node-app:domain=$domainprefix.cpp --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242

		# node - run the test against java provider
        echo "run node<->cpp joynr system integration test"
		npm run-script startconsumer --sit-node-app:domain=$domainprefix.java --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242

		# node - run the test against node provider
        echo "run node<->node joynr system integration test"
		npm run-script startconsumer --sit-node-app:domain=$domainprefix.node

		# node - run the test against node provider
        echo "run node<->node TLS joynr system integration test"
		npm run-script startconsumertls --sit-node-app:domain=$domainprefix.nodeTls

		# node - run the test against jee provider
        echo "run node<->jee joynr system integration test"
		npm run-script startconsumer --sit-node-app:domain=$domainprefix.jee --sit-node-app:cc:host=127.0.0.1 --sit-node-app:cc:port=4242

	)
done

echo "Finished Onboard RUN END TO END TEST"

# Clean up
# disabled, as the provider and the cluster controller must still be available for the jee consumer test
# kill $CPP_PROVIDER_PID
# kill $JAVA_PROVIDER_PID
# kill $NODE_PROVIDER_PID
# kill $CLUSTER_CONTROLLER_PID
# kill $MOSQUITTO_PID
kill NODE_TLS_PROVIDER_PID
wait
