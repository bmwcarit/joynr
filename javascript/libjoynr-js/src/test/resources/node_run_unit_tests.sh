###
# #%L
# %%
# Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

#!/bin/sh
set -e
#TestConsole checks for JS Test Driver, we don't use it with node
##TESTS="${project.build.testOutputDirectory}/TestConsole.js"

TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/TestCapabilityInformation.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/TestCapabilitiesRegistrar.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/TestCapabilitiesStore.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/TestParticipantIdStorage.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/arbitration/TestArbitrator.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/discovery/TestDiscoveryQos.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/arbitration/TestArbitrationStrategies.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/capabilities/discovery/TestCapabilityDiscovery.js"
# TODO: Fix JoynrMessage tests
##TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/TestJoynrMessage.js
# FIXME: this is not a jasmine test definition but a jstestdriver test definition
##TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/TestJsonParser.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/TestMessagingQos.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/TestMessagingStubFactory.js"
# TODO: Fix TestLongPollingChannelMessageReceiver for node
##TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/channel/TestLongPollingChannelMessageReceiver.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/channel/TestChannelMessagingSender.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/channel/TestChannelMessagingStub.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/channel/TestChannelMessagingSkeleton.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/channel/TestChannelMessagingStubFactory.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/inprocess/TestInProcessAddress.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/inprocess/TestInProcessMessagingStub.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/inprocess/TestInProcessMessagingSkeleton.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/inprocess/TestInProcessMessagingStubFactory.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/browser/TestBrowserMessagingStub.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/browser/TestBrowserMessagingSkeleton.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/browser/TestBrowserMessagingStubFactory.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/routing/TestMessageRouter.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/routing/TestMessageQueue.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/routing/TestLocalChannelUrlDirectory.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/webmessaging/TestWebMessagingAddress.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/webmessaging/TestWebMessagingStub.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/webmessaging/TestWebMessagingSkeleton.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/webmessaging/TestWebMessagingStubFactory.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/websocket/TestSharedWebSocket.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/websocket/TestWebSocketMessagingStub.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/websocket/TestWebSocketMessagingSkeleton.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/messaging/websocket/TestWebSocketMessagingStubFactory.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/TestDispatcher.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/types/TestRequest.js "
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/types/TestSubscriptionRequest.js "
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/TestRequestReplyManager.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/subscription/TestSubscriptionManager.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/subscription/TestSubscriptionUtil.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/dispatching/subscription/TestPublicationManager.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/provider/TestProvider.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/provider/TestProviderAttribute.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/provider/TestProviderOperation.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/provider/TestProviderEvent.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/provider/TestBroadcastOutputParameters.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/proxy/TestProxy.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/proxy/TestProxyBuilder.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/proxy/TestProxyAttribute.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/proxy/TestProxyOperation.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/proxy/TestProxyEvent.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/proxy/TestSubscriptionQos.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestUtil.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestJSONSerializer.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestTypeGenerator.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestLongTimer.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestTyping.js"
#We do not use cookies with node. Instead we are using localStorage
#TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestCookiePersistency.js"
TESTS="$TESTS ${project.build.testOutputDirectory}/joynr/util/TestInProcessStubAndSkeleton.js"

cd ${project.build.directory}/node-classes

echo "preparing the node environment incl. required dependencies"

#currently, we assume npm and node is installed on the machine running this script
#download and install all required node modules using the package.json
npm install

#so far, only files located at the ${project.build.outputDirectory} can find the installed node_modules
#make the installed node_modules available for the test-classes
ln -sf ${project.build.directory}/node-classes/node_modules ..

#check if all defined test files really exist. Jasmine-node would return 0 without running the tests otherwise.
for Test in $TESTS
do
  if [ ! -f "$Test" ]; then
      echo "File not found: $Test"
      exit 1
  fi
done

echo "running the following tests: $TESTS"

#currently, we assume npm and node is installed on the machine running this script
#${project.build.directory}/nodejs/node ${project.build.directory}/node_modules/jasmine-node/lib/jasmine-node/cli.js --requireJsSetup ${project.build.testOutputDirectory}/node_require.config.js --matchall $@ $TESTS
node ${project.build.directory}/node_modules/jasmine-node/lib/jasmine-node/cli.js --requireJsSetup ${project.build.testOutputDirectory}/node_require.config.js --matchall $@ $TESTS

exit
