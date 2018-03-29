/* jslint global:true*/

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

// because the generated code uses require('joynr') without knowing the location, it will work only
// when joynr is a submodule and is placed inside node_modules folder. In order to emulate this
// behavior the require function is adapted here in order to always return the correct joynr while
// running tests.
var mod = require("module");
var joynr = require("../classes/joynr");
req = mod.prototype.require;

mod.prototype.require = function(md) {
    if (md === "joynr") {
        return joynr;
    }
    if (md.endsWith("SmrfNode")) {
        return req("../test-classes/global/SmrfMock");
    }
    if (md.endsWith("WebSocketNode")) {
        return req("../test-classes/global/WebSocketMock");
    }
    return req.apply(this, arguments);
};

console.log("require config setup");

var ArbitrationStrategiesTest = require("../test-classes/joynr/capabilities/arbitration/ArbitrationStrategiesTest");
var ArbitratorTest = require("../test-classes/joynr/capabilities/arbitration/ArbitratorTest");
var BroadcastOutputParametersTest = require("../test-classes/joynr/provider/BroadcastOutputParametersTest");
var BrowserMessagingSkeletonTest = require("../test-classes/joynr/messaging/browser/BrowserMessagingSkeletonTest");
var BrowserMessagingStubFactoryTest = require("../test-classes/joynr/messaging/browser/BrowserMessagingStubFactoryTest");
var BrowserMessagingStubTest = require("../test-classes/joynr/messaging/browser/BrowserMessagingStubTest");
var CapabilitiesRegistrarTest = require("../test-classes/joynr/capabilities/CapabilitiesRegistrarTest");
var CapabilitiesStoreTest = require("../test-classes/joynr/capabilities/CapabilitiesStoreTest");
var CapabilitiesUtilTest = require("../test-classes/joynr/util/CapabilitiesUtilTest");
var CapabilityDiscoveryTest = require("../test-classes/joynr/capabilities/discovery/CapabilityDiscoveryTest");
var CapabilityInformationTest = require("../test-classes/joynr/capabilities/CapabilityInformationTest");
var ChannelMessagingSenderTest = require("../test-classes/joynr/messaging/channel/ChannelMessagingSenderTest");
var ChannelMessagingSkeletonTest = require("../test-classes/joynr/messaging/channel/ChannelMessagingSkeletonTest");
var ChannelMessagingStubFactoryTest = require("../test-classes/joynr/messaging/channel/ChannelMessagingStubFactoryTest");
var ChannelMessagingStubTest = require("../test-classes/joynr/messaging/channel/ChannelMessagingStubTest");
var DiscoveryQosTest = require("../test-classes/joynr/capabilities/discovery/DiscoveryQosTest");
var DispatcherTest = require("../test-classes/joynr/dispatching/DispatcherTest");
var GenerationUtilTest = require("../test-classes/joynr/util/GenerationUtilTest");
var InProcessAddressTest = require("../test-classes/joynr/messaging/inprocess/InProcessAddressTest");
var InProcessMessagingSkeletonTest = require("../test-classes/joynr/messaging/inprocess/InProcessMessagingSkeletonTest");
var InProcessMessagingStubFactoryTest = require("../test-classes/joynr/messaging/inprocess/InProcessMessagingStubFactoryTest");
var InProcessMessagingStubTest = require("../test-classes/joynr/messaging/inprocess/InProcessMessagingStubTest");
var InProcessStubAndSkeletonTest = require("../test-classes/joynr/util/InProcessStubAndSkeletonTest");
var JoynrMessageTest = require("../test-classes/joynr/messaging/JoynrMessageTest");
var JoynrLoggerTest = require("../test-classes/joynr/system/JoynrLoggerTest");
var JsonSerializerTest = require("../test-classes/joynr/util/JSONSerializerTest");
var LocalStorageNodeTest = require("../test-classes/global/LocalStorageNodeTest");
var LongPollingChannelMessageReceiverTest = require("../test-classes/joynr/messaging/channel/LongPollingChannelMessageReceiverTest");
var LongTimerTest = require("../test-classes/joynr/util/LongTimerTest");
var MemoryStorage = require("../test-classes/global/MemoryStorageTest");
var MessageQueueTest = require("../test-classes/joynr/messaging/routing/MessageQueueTest");
var MessageReplyToAddressCalculatorTest = require("../test-classes/joynr/messaging/MessageReplyToAddressCalculatorTest");
var MessageRouterTest = require("../test-classes/joynr/messaging/routing/MessageRouterTest");
var MessagingQosTest = require("../test-classes/joynr/messaging/MessagingQosTest");
var MessagingStubFactoryTest = require("../test-classes/joynr/messaging/MessagingStubFactoryTest");
var MethodUtilTest = require("../test-classes/joynr/util/MethodUtilTest");
var MqttMessagingSkeletonTest = require("../test-classes/joynr/messaging/mqtt/MqttMessagingSkeletonTest");
var MqttMessagingStubFactoryTest = require("../test-classes/joynr/messaging/mqtt/MqttMessagingStubFactoryTest");
var MqttMessagingStubTest = require("../test-classes/joynr/messaging/mqtt/MqttMessagingStubTest");
var MulticastPublicationTest = require("../test-classes/joynr/dispatching/types/MulticastPublicationTest");
var ParticipantIdStorageTest = require("../test-classes/joynr/capabilities/ParticipantIdStorageTest");
var ParticipantQueueTest = require("../test-classes/joynr/messaging/routing/ParticipantQueueTest");
var ProviderAttributeTest = require("../test-classes/joynr/provider/ProviderAttributeTest");
var ProviderBuilderTest = require("../test-classes/joynr/provider/ProviderBuilderTest");
var ProviderEventTest = require("../test-classes/joynr/provider/ProviderEventTest");
var ProviderOperationTest = require("../test-classes/joynr/provider/ProviderOperationTest");
var ProviderQosTest = require("../test-classes/joynr/provider/ProviderQosTest");
var ProviderTest = require("../test-classes/joynr/provider/ProviderTest");
var ProxyAttributeTest = require("../test-classes/joynr/proxy/ProxyAttributeTest");
var ProxyBuilderTest = require("../test-classes/joynr/proxy/ProxyBuilderTest");
var ProxyEventTest = require("../test-classes/joynr/proxy/ProxyEventTest");
var ProxyOperationTest = require("../test-classes/joynr/proxy/ProxyOperationTest");
var ProxyTest = require("../test-classes/joynr/proxy/ProxyTest");
var PublicationManagerTest = require("../test-classes/joynr/dispatching/subscription/PublicationManagerTest");
var ReplyTest = require("../test-classes/joynr/dispatching/types/ReplyTest");
var RequestReplyManagerTest = require("../test-classes/joynr/dispatching/RequestReplyManagerTest");
var RequestTest = require("../test-classes/joynr/dispatching/types/RequestTest");
var SharedWebSocketTest = require("../test-classes/joynr/messaging/websocket/SharedWebSocketTest");
var SubscriptionManagerTest = require("../test-classes/joynr/dispatching/subscription/SubscriptionManagerTest");
var SubscriptionPublicationTest = require("../test-classes/joynr/dispatching/types/SubscriptionPublicationTest");
var SubscriptionQosTest = require("../test-classes/joynr/proxy/SubscriptionQosTest");
var SubscriptionRequestTest = require("../test-classes/joynr/dispatching/types/SubscriptionRequestTest");
var SubscriptionUtilTest = require("../test-classes/joynr/dispatching/subscription/SubscriptionUtilTest");
var TtlUpliftTest = require("../test-classes/joynr/dispatching/TtlUpliftTest");
var TypeGeneratorTest = require("../test-classes/joynr/util/TypeGeneratorTest");
var TypingTest = require("../test-classes/joynr/util/TypingTest");
var UtilTest = require("../test-classes/joynr/util/UtilTest");
var WebMessagingAddressTest = require("../test-classes/joynr/messaging/webmessaging/WebMessagingAddressTest");
var WebMessagingSkeletonTest = require("../test-classes/joynr/messaging/webmessaging/WebMessagingSkeletonTest");
var WebMessagingStubFactoryTest = require("../test-classes/joynr/messaging/webmessaging/WebMessagingStubFactoryTest");
var WebMessagingStubTest = require("../test-classes/joynr/messaging/webmessaging/WebMessagingStubTest");
var WebSocketMessagingSkeletonTest = require("../test-classes/joynr/messaging/websocket/WebSocketMessagingSkeletonTest");
var WebSocketMessagingStubFactoryTest = require("../test-classes/joynr/messaging/websocket/WebSocketMessagingStubFactoryTest");
var WebSocketMessagingStubTest = require("../test-classes/joynr/messaging/websocket/WebSocketMessagingStubTest");
var WebSocketNodeTest = require("../test-classes/global/WebSocketNodeTest");
var WebSocketLibJoynrRuntimeTest = require("../test-classes/joynr/start/WebSocketLibjoynrRuntimeTest");

console.log("all tests modules loaded");
loadingFinished = true;
jasmine.execute();
