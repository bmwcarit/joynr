#!/bin/bash
###
# #%L
# %%
# Copyright (C) 2019 BMW Car IT GmbH
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

# Uncomment cmdline below to get stdout/stderr logged into
# file for online inspection when being interactively logged
# in to running container with a shell
#exec 1> /tmp/run-onboard-sit.log 2>&1

function wait_for_gcd {
    try_count=0
    max_retries=24
    while [ -z "$(echo '\n' | curl -v telnet://$1:9998 2>&1 | grep 'OK')" ]
    do
        echo "GCD $1 not started yet ..."
        try_count=$((try_count+1))
        if [ $try_count -gt $max_retries ]; then
            echo "GCD $1 failed to start in time."
            exit 1
        fi
        echo "try_count ${try_count}"
        sleep 5
    done
    echo "GCD $1 started successfully."
    return 0
}

# Give the Discovery Directory a chance to start ...
wait_for_gcd "joynr-gcd-1"
wait_for_gcd "joynr-gcd-2"

echo "SIT: Onboard RUN END TO END TEST"

echo "ENVIRONMENT"
env

# for repeated manual execution make sure there is nothing left
pkill -9 cluster 2> /dev/null
pkill -9 java 2> /dev/null
pkill -9 jsit 2> /dev/null
pkill -9 node 2> /dev/null
sleep 1

export JOYNR_LOG_LEVEL=TRACE

# fail on first error
set -e

NVM_DIR=/usr/local/nvm
. $NVM_DIR/nvm.sh
type node

DATA_DIR=/data
LOG_DIR=/tmp/logs
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

echo "SIT: adding $DOMAIN_PREFIX also to the list of consumer domain prefixes"
CONSUMER_DOMAIN_PREFIXES=$CONSUMER_DOMAIN_PREFIXES$DOMAIN_PREFIX

GBIDS[0]="joynrdefaultgbid"
GBIDS[1]="othergbid"
GBIDS[2]="joynrdefaultgbid,othergbid"
BACKEND_PREFIX[0]="onlyBackend1"
BACKEND_PREFIX[1]="onlyBackend2"
BACKEND_PREFIX[2]="bothBackends"

echo "SIT: Starting cluster controller + providers with domain prefix $DOMAIN_PREFIX"

# Prepare test environments
for i in 0 1 2
do
	cd ${CPP_HOME}
	rm -rf ${BACKEND_PREFIX[$i]}
	cp -a bin ${BACKEND_PREFIX[$i]}
	cd ${DATA_DIR}
	rm -rf sit-node-app-${BACKEND_PREFIX[$i]}
	cp -a sit-node-app sit-node-app-${BACKEND_PREFIX[$i]}
	rm -rf sit-node-app-tls-${BACKEND_PREFIX[$i]}
	cp -a sit-node-app sit-node-app-tls-${BACKEND_PREFIX[$i]}
	cd ${DATA_DIR}
	rm -rf sit-java-app-provider-${BACKEND_PREFIX[$i]}
	cp -a sit-java-app sit-java-app-provider-${BACKEND_PREFIX[$i]}
	rm -rf sit-java-app-consumer-${BACKEND_PREFIX[$i]}
	cp -a sit-java-app sit-java-app-consumer-${BACKEND_PREFIX[$i]}
done


cd ${CPP_HOME}
rm -rf failure
cp -a bin failure
cd ${DATA_DIR}
rm -rf sit-node-app-failure
cp -a sit-node-app sit-node-app-failure
cd ${DATA_DIR}
rm -rf sit-java-app-provider-failure
cp -a sit-java-app sit-java-app-provider-failure
rm -rf sit-java-app-consumer-failure
cp -a sit-java-app sit-java-app-consumer-failure

# just in case this is not mounted as a volume, create it
mkdir -p ${LOG_DIR}

# Start cluster controller
cd ${CPP_HOME}/bin
/usr/bin/cluster-controller ${DATA_DIR}/onboard-cc-messaging.settings > ${LOG_DIR}/cc.log 2>&1 &
CLUSTER_CONTROLLER_PID=$!

/usr/bin/cluster-controller ${DATA_DIR}/onboard-failure-cc-messaging.settings > ${LOG_DIR}/failure-cc.log 2>&1 &
FAILURE_CLUSTER_CONTROLLER_PID=$!

# Start provider
for i in 0 1 2
do
	echo "SIT: Starting C++ provider for domain ${BACKEND_PREFIX[$i]}_${DOMAIN_PREFIX}.cpp"
	cd ${CPP_HOME}/${BACKEND_PREFIX[$i]}
	./jsit-provider-ws -d ${BACKEND_PREFIX[$i]}_${DOMAIN_PREFIX}.cpp -r -g ${GBIDS[$i]} -G > ${LOG_DIR}/provider_cpp_${BACKEND_PREFIX[$i]}.log 2>&1 &
	CPP_PROVIDER_PID[$i]=$!

	echo "SIT: Starting Node provider for domain ${BACKEND_PREFIX[$i]}_$DOMAIN_PREFIX.node"
	cd ${DATA_DIR}/sit-node-app-${BACKEND_PREFIX[$i]}
	npm run-script startprovider --sit-node-app:domain=${BACKEND_PREFIX[$i]}_$DOMAIN_PREFIX.node --sit-node-app:gbids=${GBIDS[$i]} > ${LOG_DIR}/provider_node_${BACKEND_PREFIX[$i]}.log 2>&1 &
	NODE_PROVIDER_PID[$i]=$!

	echo "SIT: Starting Node TLS provider for domain ${BACKEND_PREFIX[$i]}_$DOMAIN_PREFIX.nodeTls"
	cd ${DATA_DIR}/sit-node-app-tls-${BACKEND_PREFIX[$i]}
	npm run-script startprovidertls --sit-node-app:domain=${BACKEND_PREFIX[$i]}_$DOMAIN_PREFIX.nodeTls --sit-node-app:gbids=${GBIDS[$i]} > ${LOG_DIR}/provider_tls_node_${BACKEND_PREFIX[$i]}.log 2>&1 &
	NODE_TLS_PROVIDER_PID[$i]=$!

	echo "SIT: Starting Java provider for domain ${BACKEND_PREFIX[$i]}_$DOMAIN_PREFIX.java"
	cd ${DATA_DIR}/sit-java-app-provider-${BACKEND_PREFIX[$i]}
	java -cp *.jar io.joynr.systemintegrationtest.ProviderApplication -d ${BACKEND_PREFIX[$i]}_$DOMAIN_PREFIX.java -r -g ${GBIDS[$i]} -G > ${LOG_DIR}/provider_java_${BACKEND_PREFIX[$i]}.log 2>&1 &
	JAVA_PROVIDER_PID[$i]=$!
done

echo "SIT: Starting C++ provider registration failure in invalid backend test"
cd ${CPP_HOME}/failure
./jsit-provider-ws -d failure.cpp -f -g "invalid" -G

echo "SIT: Starting Java provider registration failure in invalid backend test"
cd ${DATA_DIR}/sit-java-app-provider-failure
java -cp *.jar io.joynr.systemintegrationtest.ProviderApplication -d failure.java -f -g "invalid" -G

echo "SIT: Starting JS provider registration failure in unknown backend test"
cd ${DATA_DIR}/sit-node-app-failure
npm run-script startprovider --sit-node-app:domain=failure.node --sit-node-app:gbids="invalid" --sit-node-app:expectfailure="true" --sit-node-app:cc.port=4245

echo "SIT: Sleeping 20 secs to let providers initialize and register at JDS"
sleep 20

echo "SIT: Waiting for JEE Application to be started"
(
	wait_for_jee_endpoint() {
		for i in 1 2 3
		do
			retry_count=0
			max_retries=60
			while [ $retry_count -le $max_retries ]
			do
				curl --noproxy sit-jee-app-$i -f -s http://sit-jee-app-$i:8080/sit-jee-app/sit-controller/ping
				exitcode=$?
				if [ $exitcode -eq 0 ]
				then
					break
				fi
				echo "SIT: JEE application not started yet ... curl failed with exit code $exitcode"
				sleep 5
				retry_count=$(($retry_count+1))
			done

			if [ $retry_count -gt $max_retries ]
			then
				echo "SIT RESULT error: JEE application failed to start in time."
				exit 1
			fi
		done
		return 0
	}
	wait_for_jee_endpoint && echo "SIT: JEE endpoint started, continuing"
)

# ignore errors from here
set +e

for domainprefix in "${CONSUMER_DOMAIN_PREFIXES[@]}"
do
	echo "SIT: Running cpp consumer test"
	(
		for i in 0 1 2
		do
			# cpp - run the test against cpp provider
			echo "SIT: running cpp<->cpp joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.cpp"
			cd ${CPP_HOME}/${BACKEND_PREFIX[$i]}
			./jsit-consumer-ws -d ${BACKEND_PREFIX[$i]}_$domainprefix.cpp -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: cpp<->cpp joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr cpp system integration test against cpp provider failed with error code $?"
			fi
		done

		for i in 0 1 2
		do
			# cpp - run the test against java provider
			echo "SIT: running cpp<->java joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.java"
			cd ${CPP_HOME}/${BACKEND_PREFIX[$i]}
			./jsit-consumer-ws -d ${BACKEND_PREFIX[$i]}_$domainprefix.java -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: cpp<->java joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr cpp system integration test against java provider failed with error code $?"
			fi
		done

		for i in 0 1 2
		do
			## cpp - run the test against node provider
			echo "SIT: running cpp<->node joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.node"
			cd ${CPP_HOME}/${BACKEND_PREFIX[$i]}
			./jsit-consumer-ws -d ${BACKEND_PREFIX[$i]}_$domainprefix.node -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: cpp<->node joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr cpp system integration test against node provider failed with error code $?"
			fi
		done

		for i in 0 1 2
		do
			# cpp - run the test against jee provider
			cd ${CPP_HOME}/${BACKEND_PREFIX[$i]}
			echo "SIT: running cpp<->jee joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.jee"
			./jsit-consumer-ws -d ${BACKEND_PREFIX[$i]}_$domainprefix.jee -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: cpp<->jee joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr cpp system integration test against jee provider failed with error code $?"
			fi
		done

                # cpp - run failure tests
                cd ${CPP_HOME}/failure
                echo "SIT: running cpp joynr system integration test for failure on invalid gbid"
                ./jsit-consumer-ws -d failure -f -g "invalid" -G

                if [ $? -eq 0 ]
                then
                        echo "SIT: cpp joynr system integration test for failure on invalid gbid failed"
                else
                        echo "SIT RESULT SUCCESS joynr cpp system integration test for failure on invalid gbid failed as expected"
                fi

                cd ${CPP_HOME}/failure
                echo "SIT: running cpp joynr system integration test for failure on unknown gbid"
                ./jsit-consumer-ws -d failure -f -g "othergbid" -G

                if [ $? -eq 0 ]
                then
                        echo "SIT: cpp joynr system integration test for failure on unknown gbid failed"
                else
                        echo "SIT RESULT SUCCESS joynr cpp system integration test for failure on unknown gbid failed as expected"
                fi
	)


	echo "run java consumer test"
	(
		for i in 0 1 2
		do
			# java - run the test against cpp provider
			echo "SIT: run java<->cpp joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.cpp"
			cd ${DATA_DIR}/sit-java-app-consumer-${BACKEND_PREFIX[$i]}
			rm -f *.persistence_file joynr_participantIds.properties
			java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication -d ${BACKEND_PREFIX[$i]}_$domainprefix.cpp -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: java<->cpp joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr java system integration test against cpp provider failed with error code $?"
			fi
		done

		for i in 0 1 2
		do
			# java - run the test against java provider
			echo "SIT: run java<->java joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.java"
			cd ${DATA_DIR}/sit-java-app-consumer-${BACKEND_PREFIX[$i]}
			rm -f *.persistence_file joynr_participantIds.properties
			java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication -d ${BACKEND_PREFIX[$i]}_$domainprefix.java -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: java<->java joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr java system integration test against java provider failed with error code $?"
			fi
		done

		for i in 0 1 2
		do
			# java - run the test against node provider
			echo "SIT: run java<->node joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.node"
			cd ${DATA_DIR}/sit-java-app-consumer-${BACKEND_PREFIX[$i]}
			rm -f *.persistence_file joynr_participantIds.properties
			java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication -d ${BACKEND_PREFIX[$i]}_$domainprefix.node -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: java<->node joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr java system integration test against node provider failed with error code $?"
			fi
		done

		for i in 0 1 2
		do
			# java - run the test against jee provider
			echo "SIT: run java<->jee joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.jee"
			cd ${DATA_DIR}/sit-java-app-consumer-${BACKEND_PREFIX[$i]}
			rm -f *.persistence_file joynr_participantIds.properties
			java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication -d ${BACKEND_PREFIX[$i]}_$domainprefix.jee -g ${GBIDS[$i]} -G

			if [ $? -eq 0 ]
			then
				echo "SIT: java<->jee joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix succeeded"
			else
				echo "SIT RESULT ERROR joynr java system integration test against jee provider failed with error code $?"
			fi
		done

                # java - run failure tests
                echo "SIT: run java joynr system integration test for failure on invalid gbid"
                cd ${DATA_DIR}/sit-java-app-consumer-failure
                rm -f *.persistence_file joynr_participantIds.properties
                java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication -d failure -g "invalid" -f -G

                if [ $? -eq 0 ]
                then
                        echo "SIT: java joynr system integration test for failure on invalid gbid succeeded"
                else
                        echo "SIT RESULT ERROR joynr java system integration test for failure on invalid gbid failed with error code $?"
                fi

                echo "SIT: run java joynr system integration test for failure on unknown gbid"
                cd ${DATA_DIR}/sit-java-app-consumer-failure
                rm -f *.persistence_file joynr_participantIds.properties
                java -cp *.jar io.joynr.systemintegrationtest.ConsumerApplication -d failure -g "othergbid" -f -G

                if [ $? -eq 0 ]
                then
                        echo "SIT: java joynr system integration test for failure on unknown gbid succeeded"
                else
                        echo "SIT RESULT ERROR joynr java system integration test for failure on unknown gbid failed with error code $?"
                fi
	)

	echo "run node consumer test"
	(
		for i in 0 1 2
		do
			# node - run the test against cpp provider
			echo "SIT: run node<->cpp joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.cpp"
			cd ${DATA_DIR}/sit-node-app-${BACKEND_PREFIX[$i]}
			npm run-script startconsumer \
				--sit-node-app:domain=${BACKEND_PREFIX[$i]}_$domainprefix.cpp \
				--sit-node-app:cc:host=127.0.0.1 \
				--sit-node-app:cc:port=4242 \
				--sit-node-app:gbids=${GBIDS[$i]}
		done

		for i in 0 1 2
		do
			# node - run the test against java provider
			echo "SIT: run node<->java joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.java"
			cd ${DATA_DIR}/sit-node-app-${BACKEND_PREFIX[$i]}
			npm run-script startconsumer \
				--sit-node-app:domain=${BACKEND_PREFIX[$i]}_$domainprefix.java \
				--sit-node-app:cc:host=127.0.0.1 \
				--sit-node-app:cc:port=4242 \
				--sit-node-app:gbids=${GBIDS[$i]}
		done

		for i in 0 1 2
		do
			# node - run the test against node provider
			echo "SIT: run node<->node joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.node"
			cd ${DATA_DIR}/sit-node-app-${BACKEND_PREFIX[$i]}
			npm run-script startconsumer \
				--sit-node-app:domain=${BACKEND_PREFIX[$i]}_$domainprefix.node \
				--sit-node-app:cc:host=127.0.0.1 \
				--sit-node-app:cc:port=4242 \
				--sit-node-app:gbids=${GBIDS[$i]}
		done

		for i in 0 1 2
		do
			# node - run the test against node provider with tls
			echo "SIT: run node<->node joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.nodeTls"
			cd ${DATA_DIR}/sit-node-app-tls-${BACKEND_PREFIX[$i]}
			npm run-script startconsumertls \
				--sit-node-app:domain=${BACKEND_PREFIX[$i]}_$domainprefix.nodeTls \
				--sit-node-app:cc:host=127.0.0.1 \
				--sit-node-app:cc:port=4243 \
				--sit-node-app:gbids=${GBIDS[$i]}
		done

		for i in 0 1 2
		do
			# node - run the test against jee provider
			echo "SIT: run node<->jee joynr system integration test for domain ${BACKEND_PREFIX[$i]}_$domainprefix.jee"
			cd ${DATA_DIR}/sit-node-app-${BACKEND_PREFIX[$i]}
			npm run-script startconsumer \
				--sit-node-app:domain=${BACKEND_PREFIX[$i]}_$domainprefix.jee \
				--sit-node-app:cc:host=127.0.0.1 \
				--sit-node-app:cc:port=4242 \
				--sit-node-app:gbids=${GBIDS[$i]}
		done

                echo "SIT: run node joynr system integration test for failure on invalid gbid"
                cd ${DATA_DIR}/sit-node-app-failure
                npm run-script startconsumer \
                        --sit-node-app:domain=failure \
                        --sit-node-app:cc:host=127.0.0.1 \
                        --sit-node-app:cc:port=4245 \
                        --sit-node-app:gbids="invalid" \
                        --sit-node-app:expectfailure="true"

                echo "SIT: run node joynr system integration test for failure on unknown gbid"
                cd ${DATA_DIR}/sit-node-app-failure
                npm run-script startconsumer \
                        --sit-node-app:domain=failure \
                        --sit-node-app:cc:host=127.0.0.1 \
                        --sit-node-app:cc:port=4245 \
                        --sit-node-app:gbids="othergbid" \
                        --sit-node-app:expectfailure="true"
	)
done

echo "SIT: Finished Onboard RUN END TO END TEST"

# Clean up
# disabled, as the provider and the cluster controller must still be available for the jee consumer test
# kill $CPP_PROVIDER_PID
# kill $JAVA_PROVIDER_PID
# kill $NODE_PROVIDER_PID
# kill $CLUSTER_CONTROLLER_PID
# kill $MOSQUITTO_PID
set +e
for i in 0 1 2
do
	echo "kill -9 ${NODE_TLS_PROVIDER_PID[$i]}"
	kill -9 ${NODE_TLS_PROVIDER_PID[$i]}
done
set -e
wait
