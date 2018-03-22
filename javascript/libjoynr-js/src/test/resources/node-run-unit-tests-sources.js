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

require("./node-unit-test-helper");

var ArbitrationStrategiesTest = require("../../test/js/joynr/capabilities/arbitration/ArbitrationStrategiesTest");
var ArbitratorTest = require("../../test/js/joynr/capabilities/arbitration/ArbitratorTest");
var BroadcastOutputParametersTest = require("../../test/js/joynr/provider/BroadcastOutputParametersTest");
var BrowserMessagingSkeletonTest = require("../../test/js/joynr/messaging/browser/BrowserMessagingSkeletonTest");
var BrowserMessagingStubFactoryTest = require("../../test/js/joynr/messaging/browser/BrowserMessagingStubFactoryTest");
var BrowserMessagingStubTest = require("../../test/js/joynr/messaging/browser/BrowserMessagingStubTest");
var CapabilitiesRegistrarTest = require("../../test/js/joynr/capabilities/CapabilitiesRegistrarTest");
var CapabilitiesStoreTest = require("../../test/js/joynr/capabilities/CapabilitiesStoreTest");
var CapabilitiesUtilTest = require("../../test/js/joynr/util/CapabilitiesUtilTest");
var CapabilityDiscoveryTest = require("../../test/js/joynr/capabilities/discovery/CapabilityDiscoveryTest");
var CapabilityInformationTest = require("../../test/js/joynr/capabilities/CapabilityInformationTest");
var ChannelMessagingSenderTest = require("../../test/js/joynr/messaging/channel/ChannelMessagingSenderTest");
var ChannelMessagingSkeletonTest = require("../../test/js/joynr/messaging/channel/ChannelMessagingSkeletonTest");
var ChannelMessagingStubFactoryTest = require("../../test/js/joynr/messaging/channel/ChannelMessagingStubFactoryTest");
var ChannelMessagingStubTest = require("../../test/js/joynr/messaging/channel/ChannelMessagingStubTest");
var DiscoveryQosTest = require("../../test/js/joynr/capabilities/discovery/DiscoveryQosTest");
var DispatcherTest = require("../../test/js/joynr/dispatching/DispatcherTest");
var GenerationUtilTest = require("../../test/js/joynr/util/GenerationUtilTest");
var InProcessAddressTest = require("../../test/js/joynr/messaging/inprocess/InProcessAddressTest");
var InProcessMessagingSkeletonTest = require("../../test/js/joynr/messaging/inprocess/InProcessMessagingSkeletonTest");
var InProcessMessagingStubFactoryTest = require("../../test/js/joynr/messaging/inprocess/InProcessMessagingStubFactoryTest");
var InProcessMessagingStubTest = require("../../test/js/joynr/messaging/inprocess/InProcessMessagingStubTest");
var InProcessStubAndSkeletonTest = require("../../test/js/joynr/util/InProcessStubAndSkeletonTest");
var JoynrMessageTest = require("../../test/js/joynr/messaging/JoynrMessageTest");
var JoynrLoggerTest = require("../../test/js/joynr/system/JoynrLoggerTest");
var JsonSerializerTest = require("../../test/js/joynr/util/JSONSerializerTest");
var LocalStorageNodeTest = require("../../test/js/global/LocalStorageNodeTest");
var LongPollingChannelMessageReceiverTest = require("../../test/js/joynr/messaging/channel/LongPollingChannelMessageReceiverTest");
var LongTimerTest = require("../../test/js/joynr/util/LongTimerTest");
var MemoryStorage = require("../../test/js/global/MemoryStorageTest");
var MessageQueueTest = require("../../test/js/joynr/messaging/routing/MessageQueueTest");
var MessageReplyToAddressCalculatorTest = require("../../test/js/joynr/messaging/MessageReplyToAddressCalculatorTest");
var MessageRouterTest = require("../../test/js/joynr/messaging/routing/MessageRouterTest");
var MessagingQosTest = require("../../test/js/joynr/messaging/MessagingQosTest");
var MessagingStubFactoryTest = require("../../test/js/joynr/messaging/MessagingStubFactoryTest");
var MethodUtilTest = require("../../test/js/joynr/util/MethodUtilTest");
var MqttMessagingSkeletonTest = require("../../test/js/joynr/messaging/mqtt/MqttMessagingSkeletonTest");
var MqttMessagingStubFactoryTest = require("../../test/js/joynr/messaging/mqtt/MqttMessagingStubFactoryTest");
var MqttMessagingStubTest = require("../../test/js/joynr/messaging/mqtt/MqttMessagingStubTest");
var MulticastPublicationTest = require("../../test/js/joynr/dispatching/types/MulticastPublicationTest");
var ParticipantIdStorageTest = require("../../test/js/joynr/capabilities/ParticipantIdStorageTest");
var ParticipantQueueTest = require("../../test/js/joynr/messaging/routing/ParticipantQueueTest");
var ProviderAttributeTest = require("../../test/js/joynr/provider/ProviderAttributeTest");
var ProviderBuilderTest = require("../../test/js/joynr/provider/ProviderBuilderTest");
var ProviderEventTest = require("../../test/js/joynr/provider/ProviderEventTest");
var ProviderOperationTest = require("../../test/js/joynr/provider/ProviderOperationTest");
var ProviderQosTest = require("../../test/js/joynr/provider/ProviderQosTest");
var ProviderTest = require("../../test/js/joynr/provider/ProviderTest");
var ProxyAttributeTest = require("../../test/js/joynr/proxy/ProxyAttributeTest");
var ProxyBuilderTest = require("../../test/js/joynr/proxy/ProxyBuilderTest");
var ProxyEventTest = require("../../test/js/joynr/proxy/ProxyEventTest");
var ProxyOperationTest = require("../../test/js/joynr/proxy/ProxyOperationTest");
var ProxyTest = require("../../test/js/joynr/proxy/ProxyTest");
var PublicationManagerTest = require("../../test/js/joynr/dispatching/subscription/PublicationManagerTest");
var ReplyTest = require("../../test/js/joynr/dispatching/types/ReplyTest");
var RequestReplyManagerTest = require("../../test/js/joynr/dispatching/RequestReplyManagerTest");
var RequestTest = require("../../test/js/joynr/dispatching/types/RequestTest");
var SharedWebSocketTest = require("../../test/js/joynr/messaging/websocket/SharedWebSocketTest");
var SubscriptionManagerTest = require("../../test/js/joynr/dispatching/subscription/SubscriptionManagerTest");
var SubscriptionPublicationTest = require("../../test/js/joynr/dispatching/types/SubscriptionPublicationTest");
var SubscriptionQosTest = require("../../test/js/joynr/proxy/SubscriptionQosTest");
var SubscriptionRequestTest = require("../../test/js/joynr/dispatching/types/SubscriptionRequestTest");
var SubscriptionUtilTest = require("../../test/js/joynr/dispatching/subscription/SubscriptionUtilTest");
var TtlUpliftTest = require("../../test/js/joynr/dispatching/TtlUpliftTest");
var TypeGeneratorTest = require("../../test/js/joynr/util/TypeGeneratorTest");
var TypingTest = require("../../test/js/joynr/util/TypingTest");
var UtilTest = require("../../test/js/joynr/util/UtilTest");
var WebMessagingAddressTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingAddressTest");
var WebMessagingSkeletonTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingSkeletonTest");
var WebMessagingStubFactoryTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingStubFactoryTest");
var WebMessagingStubTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingStubTest");
var WebSocketMessagingSkeletonTest = require("../../test/js/joynr/messaging/websocket/WebSocketMessagingSkeletonTest");
var WebSocketMessagingStubFactoryTest = require("../../test/js/joynr/messaging/websocket/WebSocketMessagingStubFactoryTest");
var WebSocketMessagingStubTest = require("../../test/js/joynr/messaging/websocket/WebSocketMessagingStubTest");
var WebSocketNodeTest = require("../../test/js/global/WebSocketNodeTest");
var WebSocketLibJoynrRuntimeTest = require("../../test/js/joynr/start/WebSocketLibjoynrRuntimeTest");
