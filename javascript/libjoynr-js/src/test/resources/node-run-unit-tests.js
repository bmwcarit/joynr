/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

// run Jasmine 2.x unit tests via node

console.log("joynr Jasmine 2.x node unit tests");

var Jasmine = require("jasmine");
var jasmine = new Jasmine();
jasmine.loadConfigFile("spec/support/jasmine.json");

console.log("Jasmine version: " + jasmine.version);
var requirejs = require("requirejs");
console.log("loaded requirejs");

requirejs.onError =
    function(err) {
        // report the error as a failing jasmine test
        describe(
            "requirejs",
            function() {
                it(
                    "global.errback",
                    function() {
                        expect(
                            "a "
                            + err.requireType
                            + " was thrown to the global requirejs error handler, required modules: "
                            + err.requireModules.toString()).toBeFalsy();
                });
        });
        // overwrite default behavior of joynr: do not throw the error, instead just print it
        console.error(err);
        process.exit(1);
    };

requirejs.config({
    //nodeRequire : require,
    baseUrl : "${project.build.outputDirectory}",
    paths : {
        "JsonParser" : "lib/JsonParser",
        "uuid" : "lib/uuid-annotated",
        "joynr/system/ConsoleAppender" : "joynr/system/ConsoleAppenderNode",
        "joynr/security/PlatformSecurityManager" : "joynr/security/PlatformSecurityManagerNode",
        "global/LocalStorage" : "../test-classes/global/LocalStorageNodeTests",
        "atmosphere" : "lib/atmosphereNode",
        "log4javascriptDependency" : "lib/log4javascriptNode",
        "global/WebSocket" : "../test-classes/global/WebSocketMock",
        "global/WaitsFor" : "../test-classes/global/WaitsFor",
        "joynr/Runtime" : "joynr/Runtime.inprocess",
        "joynr/tests" : "../test-classes/joynr/tests",
        "joynr/types" : "../classes/joynr/types",
        "joynr/types/TestTypes" : "../test-classes/joynr/types/TestTypes",
        "joynr/types/TestTypesWithoutVersion" : "../test-classes/joynr/types/TestTypesWithoutVersion",
        "joynr/vehicle" : "../test-classes/joynr/vehicle",
        "joynr/datatypes" : "../test-classes/joynr/datatypes",
        "joynr/provisioning" : "../test-classes/joynr/provisioning",
        "test/data" : "../test-classes/test/data",
        "Date" : "../test-classes/global/Date",

        // stuff from require.config.node.js not yet included here
        //"global/LocalStorage" : "global/LocalStorageNode",
        "global/XMLHttpRequest" : "global/XMLHttpRequestNode",
        "global/WebsocketNodeModule" : "global/WebSocketNode",
        //"global/WebSocket" : "global/WebSocketNode",

        "tests" : "../test-classes"
    }
});

console.log("requirejs config setup");

requirejs([ 
    "tests/joynr/provider/ProviderOperationTest",
    "tests/joynr/provider/ProviderTest",
    "tests/joynr/provider/BroadcastOutputParametersTest",
    "tests/joynr/provider/ProviderAttributeTest",
    "tests/joynr/provider/ProviderQosTest",
    "tests/joynr/provider/ProviderBuilderTest",
    "tests/joynr/provider/ProviderEventTest",
    "tests/joynr/messaging/webmessaging/WebMessagingSkeletonTest",
    "tests/joynr/messaging/webmessaging/WebMessagingStubFactoryTest",
    "tests/joynr/messaging/webmessaging/WebMessagingAddressTest",
    "tests/joynr/messaging/webmessaging/WebMessagingStubTest",
    "tests/joynr/messaging/channel/ChannelMessagingStubTest",
    "tests/joynr/messaging/channel/ChannelMessagingStubFactoryTest",
    "tests/joynr/messaging/channel/LongPollingChannelMessageReceiverTest",
    "tests/joynr/messaging/channel/ChannelMessagingSkeletonTest",
    "tests/joynr/messaging/channel/ChannelMessagingSenderTest",
    "tests/joynr/messaging/mqtt/MqttMessagingSkeletonTest",
    "tests/joynr/messaging/mqtt/MqttMessagingStubFactoryTest",
    "tests/joynr/messaging/mqtt/MqttMessagingStubTest",
    "tests/joynr/messaging/routing/MessageQueueTest",
    "tests/joynr/messaging/routing/MessageRouterTest",
    "tests/joynr/messaging/inprocess/InProcessMessagingStubFactoryTest",
    "tests/joynr/messaging/inprocess/InProcessMessagingSkeletonTest",
    "tests/joynr/messaging/inprocess/InProcessAddressTest",
    "tests/joynr/messaging/inprocess/InProcessMessagingStubTest",
    //"tests/joynr/messaging/JsonParserTest",
    "tests/joynr/messaging/browser/BrowserMessagingStubFactoryTest",
    "tests/joynr/messaging/browser/BrowserMessagingSkeletonTest",
    "tests/joynr/messaging/browser/BrowserMessagingStubTest",
    "tests/joynr/messaging/websocket/WebSocketMessagingSkeletonTest",
    "tests/joynr/messaging/websocket/SharedWebSocketTest",
    "tests/joynr/messaging/websocket/WebSocketMessagingStubFactoryTest",
    "tests/joynr/messaging/websocket/WebSocketMessagingStubTest",
    "tests/joynr/messaging/MessagingStubFactoryTest",
    "tests/joynr/messaging/JoynrMessageTest",
    "tests/joynr/messaging/MessagingQosTest",
    "tests/joynr/messaging/MessageReplyToAddressCalculatorTest",
    "tests/joynr/system/DistributedLoggingAppenderFactoryTest",
    //"tests/joynr/system/LoggingManagerTest",
    "tests/joynr/system/DistributedLoggingAppenderTest",
    "tests/joynr/proxy/ProxyAttributeTest",
    "tests/joynr/proxy/ProxyOperationTest",
    "tests/joynr/proxy/ProxyTest",
    "tests/joynr/proxy/ProxyEventTest",
    "tests/joynr/proxy/SubscriptionQosTest",
    "tests/joynr/proxy/ProxyBuilderTest",
    //"tests/joynr/start/InProcessRuntimeTest",
    "tests/joynr/util/CapabilitiesUtilTest",
    "tests/joynr/util/InProcessStubAndSkeletonTest",
    "tests/joynr/util/UtilTest",
    "tests/joynr/util/JSONSerializerTest",
    "tests/joynr/util/TypingTest",
    "tests/joynr/util/LongTimerTest",
    "tests/joynr/util/TypeGeneratorTest",
    "tests/joynr/capabilities/discovery/DiscoveryQosTest",
    "tests/joynr/capabilities/discovery/CapabilityDiscoveryTest",
    "tests/joynr/capabilities/CapabilityInformationTest",
    "tests/joynr/capabilities/arbitration/ArbitrationStrategiesTest",
    "tests/joynr/capabilities/arbitration/ArbitratorTest",
    "tests/joynr/capabilities/CapabilitiesRegistrarTest",
    "tests/joynr/capabilities/CapabilitiesStoreTest",
    "tests/joynr/capabilities/ParticipantIdStorageTest",
    "tests/joynr/dispatching/types/MulticastPublicationTest",
    "tests/joynr/dispatching/types/SubscriptionPublicationTest",
    "tests/joynr/dispatching/types/RequestTest",
    "tests/joynr/dispatching/types/SubscriptionRequestTest",
    "tests/joynr/dispatching/types/ReplyTest",
    "tests/joynr/dispatching/subscription/SubscriptionUtilTest",
    "tests/joynr/dispatching/subscription/PublicationManagerTest",
    "tests/joynr/dispatching/subscription/SubscriptionManagerTest",
    "tests/joynr/dispatching/DispatcherTest",
    "tests/joynr/dispatching/TtlUpliftTest",
    "tests/joynr/dispatching/RequestReplyManagerTest",
    "tests/global/LocalStorageNodeTest",
    "tests/global/WebSocketNodeTest"
], function() {
    console.log("all tests modules loaded");

    loadingFinished = true;
    jasmine.execute();
});
