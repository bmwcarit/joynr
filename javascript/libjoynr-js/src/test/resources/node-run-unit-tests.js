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
var mod = require('module');
var joynr = require('../classes/joynr');
req = mod.prototype.require;

mod.prototype.require = function (md) {
    if (md === 'joynr') {
        return joynr;
    }
    if (md.endsWith('WebSocketNode')) {
        return req('../test-classes/global/WebSocketMock');
    }
    return req.apply(this, arguments);
}
console.log('require config setup');
var ProviderOperationTest = require('../test-classes/joynr/provider/ProviderOperationTest');
var ProviderTest = require('../test-classes/joynr/provider/ProviderTest');
var BroadcastOutputParametersTest = require('../test-classes/joynr/provider/BroadcastOutputParametersTest');
var ProviderAttributeTest = require('../test-classes/joynr/provider/ProviderAttributeTest');
var ProviderQosTest = require('../test-classes/joynr/provider/ProviderQosTest');
var ProviderBuilderTest = require('../test-classes/joynr/provider/ProviderBuilderTest');
var ProviderEventTest = require('../test-classes/joynr/provider/ProviderEventTest');
var WebMessagingSkeletonTest = require('../test-classes/joynr/messaging/webmessaging/WebMessagingSkeletonTest');
var WebMessagingStubFactoryTest = require('../test-classes/joynr/messaging/webmessaging/WebMessagingStubFactoryTest');
var WebMessagingAddressTest = require('../test-classes/joynr/messaging/webmessaging/WebMessagingAddressTest');
var WebMessagingStubTest = require('../test-classes/joynr/messaging/webmessaging/WebMessagingStubTest');
var ChannelMessagingStubTest = require('../test-classes/joynr/messaging/channel/ChannelMessagingStubTest');
var ChannelMessagingStubFactoryTest = require('../test-classes/joynr/messaging/channel/ChannelMessagingStubFactoryTest');
var LongPollingChannelMessageReceiverTest = require('../test-classes/joynr/messaging/channel/LongPollingChannelMessageReceiverTest');
var ChannelMessagingSkeletonTest = require('../test-classes/joynr/messaging/channel/ChannelMessagingSkeletonTest');
var ChannelMessagingSenderTest = require('../test-classes/joynr/messaging/channel/ChannelMessagingSenderTest');
var MqttMessagingSkeletonTest = require('../test-classes/joynr/messaging/mqtt/MqttMessagingSkeletonTest');
var MqttMessagingStubFactoryTest = require('../test-classes/joynr/messaging/mqtt/MqttMessagingStubFactoryTest');
var MqttMessagingStubTest = require('../test-classes/joynr/messaging/mqtt/MqttMessagingStubTest');
var MessageQueueTest = require('../test-classes/joynr/messaging/routing/MessageQueueTest');
var MessageRouterTest = require('../test-classes/joynr/messaging/routing/MessageRouterTest');
var InProcessMessagingStubFactoryTest = require('../test-classes/joynr/messaging/inprocess/InProcessMessagingStubFactoryTest');
var InProcessMessagingSkeletonTest = require('../test-classes/joynr/messaging/inprocess/InProcessMessagingSkeletonTest');
var InProcessAddressTest = require('../test-classes/joynr/messaging/inprocess/InProcessAddressTest');
var InProcessMessagingStubTest = require('../test-classes/joynr/messaging/inprocess/InProcessMessagingStubTest');
var BrowserMessagingStubFactoryTest = require('../test-classes/joynr/messaging/browser/BrowserMessagingStubFactoryTest');
var BrowserMessagingSkeletonTest = require('../test-classes/joynr/messaging/browser/BrowserMessagingSkeletonTest');
var BrowserMessagingStubTest = require('../test-classes/joynr/messaging/browser/BrowserMessagingStubTest');
var WebSocketMessagingSkeletonTest = require('../test-classes/joynr/messaging/websocket/WebSocketMessagingSkeletonTest');
var SharedWebSocketTest = require('../test-classes/joynr/messaging/websocket/SharedWebSocketTest');
var WebSocketMessagingStubFactoryTest = require('../test-classes/joynr/messaging/websocket/WebSocketMessagingStubFactoryTest');
var WebSocketMessagingStubTest = require('../test-classes/joynr/messaging/websocket/WebSocketMessagingStubTest');
var MessagingStubFactoryTest = require('../test-classes/joynr/messaging/MessagingStubFactoryTest');
var JoynrMessageTest = require('../test-classes/joynr/messaging/JoynrMessageTest');
var MessagingQosTest = require('../test-classes/joynr/messaging/MessagingQosTest');
var MessageReplyToAddressCalculatorTest = require('../test-classes/joynr/messaging/MessageReplyToAddressCalculatorTest');
var DistributedLoggingAppenderFactoryTest = require('../test-classes/joynr/system/DistributedLoggingAppenderFactoryTest');
var DistributedLoggingAppenderTest = require('../test-classes/joynr/system/DistributedLoggingAppenderTest');
var ProxyAttributeTest = require('../test-classes/joynr/proxy/ProxyAttributeTest');
var ProxyOperationTest = require('../test-classes/joynr/proxy/ProxyOperationTest');
var ProxyTest = require('../test-classes/joynr/proxy/ProxyTest');
var ProxyEventTest = require('../test-classes/joynr/proxy/ProxyEventTest');
var SubscriptionQosTest = require('../test-classes/joynr/proxy/SubscriptionQosTest');
var ProxyBuilderTest = require('../test-classes/joynr/proxy/ProxyBuilderTest');
var CapabilitiesUtilTest = require('../test-classes/joynr/util/CapabilitiesUtilTest');
var InProcessStubAndSkeletonTest = require('../test-classes/joynr/util/InProcessStubAndSkeletonTest');
var UtilTest = require('../test-classes/joynr/util/UtilTest');
var JsonSerializerTest = require('../test-classes/joynr/util/JSONSerializerTest');
var TypingTest = require('../test-classes/joynr/util/TypingTest');
var LongTimerTest = require('../test-classes/joynr/util/LongTimerTest');
var TypeGeneratorTest = require('../test-classes/joynr/util/TypeGeneratorTest');
var DiscoveryQosTest = require('../test-classes/joynr/capabilities/discovery/DiscoveryQosTest');
var CapabilityDiscoveryTest = require('../test-classes/joynr/capabilities/discovery/CapabilityDiscoveryTest');
var CapabilityInformationTest = require('../test-classes/joynr/capabilities/CapabilityInformationTest');
var ArbitrationStrategiesTest = require('../test-classes/joynr/capabilities/arbitration/ArbitrationStrategiesTest');
var ArbitratorTest = require('../test-classes/joynr/capabilities/arbitration/ArbitratorTest');
var CapabilitiesRegistrarTest = require('../test-classes/joynr/capabilities/CapabilitiesRegistrarTest');
var CapabilitiesStoreTest = require('../test-classes/joynr/capabilities/CapabilitiesStoreTest');
var ParticipantIdStorageTest = require('../test-classes/joynr/capabilities/ParticipantIdStorageTest');
var MulticastPublicationTest = require('../test-classes/joynr/dispatching/types/MulticastPublicationTest');
var SubscriptionPublicationTest = require('../test-classes/joynr/dispatching/types/SubscriptionPublicationTest');
var RequestTest = require('../test-classes/joynr/dispatching/types/RequestTest');
var SubscriptionRequestTest = require('../test-classes/joynr/dispatching/types/SubscriptionRequestTest');
var ReplyTest = require('../test-classes/joynr/dispatching/types/ReplyTest');
var SubscriptionUtilTest = require('../test-classes/joynr/dispatching/subscription/SubscriptionUtilTest');
var PublicationManagerTest = require('../test-classes/joynr/dispatching/subscription/PublicationManagerTest');
var SubscriptionManagerTest = require('../test-classes/joynr/dispatching/subscription/SubscriptionManagerTest');
var DispatcherTest = require('../test-classes/joynr/dispatching/DispatcherTest');
var TtlUpliftTest = require('../test-classes/joynr/dispatching/TtlUpliftTest');
var RequestReplyManagerTest = require('../test-classes/joynr/dispatching/RequestReplyManagerTest');
var LocalStorageNodeTest = require('../test-classes/global/LocalStorageNodeTest');
var WebSocketNodeTest = require('../test-classes/global/WebSocketNodeTest');
(function () {
    console.log("all tests modules loaded");
    loadingFinished = true;
    jasmine.execute();
}(ProviderOperationTest, ProviderTest, BroadcastOutputParametersTest, ProviderAttributeTest, ProviderQosTest, ProviderBuilderTest, ProviderEventTest, WebMessagingSkeletonTest, WebMessagingStubFactoryTest, WebMessagingAddressTest, WebMessagingStubTest, ChannelMessagingStubTest, ChannelMessagingStubFactoryTest, LongPollingChannelMessageReceiverTest, ChannelMessagingSkeletonTest, ChannelMessagingSenderTest, MqttMessagingSkeletonTest, MqttMessagingStubFactoryTest, MqttMessagingStubTest, MessageQueueTest, MessageRouterTest, InProcessMessagingStubFactoryTest, InProcessMessagingSkeletonTest, InProcessAddressTest, InProcessMessagingStubTest, BrowserMessagingStubFactoryTest, BrowserMessagingSkeletonTest, BrowserMessagingStubTest, WebSocketMessagingSkeletonTest, SharedWebSocketTest, WebSocketMessagingStubFactoryTest, WebSocketMessagingStubTest, MessagingStubFactoryTest, JoynrMessageTest, MessagingQosTest, MessageReplyToAddressCalculatorTest, DistributedLoggingAppenderFactoryTest, DistributedLoggingAppenderTest, ProxyAttributeTest, ProxyOperationTest, ProxyTest, ProxyEventTest, SubscriptionQosTest, ProxyBuilderTest, CapabilitiesUtilTest, InProcessStubAndSkeletonTest, UtilTest, JsonSerializerTest, TypingTest, LongTimerTest, TypeGeneratorTest, DiscoveryQosTest, CapabilityDiscoveryTest, CapabilityInformationTest, ArbitrationStrategiesTest, ArbitratorTest, CapabilitiesRegistrarTest, CapabilitiesStoreTest, ParticipantIdStorageTest, MulticastPublicationTest, SubscriptionPublicationTest, RequestTest, SubscriptionRequestTest, ReplyTest, SubscriptionUtilTest, PublicationManagerTest, SubscriptionManagerTest, DispatcherTest, TtlUpliftTest, RequestReplyManagerTest, LocalStorageNodeTest, WebSocketNodeTest));
