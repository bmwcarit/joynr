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

require("../js/node-unit-test-helper");

let ArbitrationStrategiesTest = require("../../test/js/joynr/capabilities/arbitration/ArbitrationStrategiesTest");
let ArbitratorTest = require("../../test/js/joynr/capabilities/arbitration/ArbitratorTest");
let BroadcastOutputParametersTest = require("../../test/js/joynr/provider/BroadcastOutputParametersTest");
let BrowserMessagingSkeletonTest = require("../../test/js/joynr/messaging/browser/BrowserMessagingSkeletonTest");
let BrowserMessagingStubFactoryTest = require("../../test/js/joynr/messaging/browser/BrowserMessagingStubFactoryTest");
let BrowserMessagingStubTest = require("../../test/js/joynr/messaging/browser/BrowserMessagingStubTest");
let CapabilitiesRegistrarTest = require("../../test/js/joynr/capabilities/CapabilitiesRegistrarTest");
let CapabilitiesStoreTest = require("../../test/js/joynr/capabilities/CapabilitiesStoreTest");
let CapabilitiesUtilTest = require("../../test/js/joynr/util/CapabilitiesUtilTest");
let CapabilityDiscoveryTest = require("../../test/js/joynr/capabilities/discovery/CapabilityDiscoveryTest");
let CapabilityInformationTest = require("../../test/js/joynr/capabilities/CapabilityInformationTest");
let DiscoveryQosTest = require("../../test/js/joynr/capabilities/discovery/DiscoveryQosTest");
let DispatcherTest = require("../../test/js/joynr/dispatching/DispatcherTest");
let GenerationUtilTest = require("../../test/js/joynr/util/GenerationUtilTest");
let InProcessAddressTest = require("../../test/js/joynr/messaging/inprocess/InProcessAddressTest");
let InProcessMessagingSkeletonTest = require("../../test/js/joynr/messaging/inprocess/InProcessMessagingSkeletonTest");
let InProcessMessagingStubFactoryTest = require("../../test/js/joynr/messaging/inprocess/InProcessMessagingStubFactoryTest");
let InProcessMessagingStubTest = require("../../test/js/joynr/messaging/inprocess/InProcessMessagingStubTest");
let InProcessStubAndSkeletonTest = require("../../test/js/joynr/util/InProcessStubAndSkeletonTest");
let JoynrMessageTest = require("../../test/js/joynr/messaging/JoynrMessageTest");
let JoynrLoggerTest = require("../../test/js/joynr/system/JoynrLoggerTest");
let JsonSerializerTest = require("../../test/js/joynr/util/JSONSerializerTest");
let LocalStorageNodeTest = require("../../test/js/global/LocalStorageNodeTest");
let LongTimerTest = require("../../test/js/joynr/util/LongTimerTest");
let MemoryStorage = require("../../test/js/global/MemoryStorageTest");
let MessageQueueTest = require("../../test/js/joynr/messaging/routing/MessageQueueTest");
let MessageReplyToAddressCalculatorTest = require("../../test/js/joynr/messaging/MessageReplyToAddressCalculatorTest");
let MessageRouterTest = require("../../test/js/joynr/messaging/routing/MessageRouterTest");
let MessagingQosTest = require("../../test/js/joynr/messaging/MessagingQosTest");
let MessagingStubFactoryTest = require("../../test/js/joynr/messaging/MessagingStubFactoryTest");
let MethodUtilTest = require("../../test/js/joynr/util/MethodUtilTest");
let MqttMessagingSkeletonTest = require("../../test/js/joynr/messaging/mqtt/MqttMessagingSkeletonTest");
let MqttMessagingStubFactoryTest = require("../../test/js/joynr/messaging/mqtt/MqttMessagingStubFactoryTest");
let MqttMessagingStubTest = require("../../test/js/joynr/messaging/mqtt/MqttMessagingStubTest");
let MulticastPublicationTest = require("../../test/js/joynr/dispatching/types/MulticastPublicationTest");
let ParticipantIdStorageTest = require("../../test/js/joynr/capabilities/ParticipantIdStorageTest");
let ParticipantQueueTest = require("../../test/js/joynr/messaging/routing/ParticipantQueueTest");
let ProviderAttributeTest = require("../../test/js/joynr/provider/ProviderAttributeTest");
let ProviderBuilderTest = require("../../test/js/joynr/provider/ProviderBuilderTest");
let ProviderEventTest = require("../../test/js/joynr/provider/ProviderEventTest");
let ProviderOperationTest = require("../../test/js/joynr/provider/ProviderOperationTest");
let ProviderQosTest = require("../../test/js/joynr/provider/ProviderQosTest");
let ProviderTest = require("../../test/js/joynr/provider/ProviderTest");
let ProxyAttributeTest = require("../../test/js/joynr/proxy/ProxyAttributeTest");
let ProxyBuilderTest = require("../../test/js/joynr/proxy/ProxyBuilderTest");
let ProxyEventTest = require("../../test/js/joynr/proxy/ProxyEventTest");
let ProxyOperationTest = require("../../test/js/joynr/proxy/ProxyOperationTest");
let ProxyTest = require("../../test/js/joynr/proxy/ProxyTest");
let PublicationManagerTest = require("../../test/js/joynr/dispatching/subscription/PublicationManagerTest");
let ReplyTest = require("../../test/js/joynr/dispatching/types/ReplyTest");
let RequestReplyManagerTest = require("../../test/js/joynr/dispatching/RequestReplyManagerTest");
let RequestTest = require("../../test/js/joynr/dispatching/types/RequestTest");
let SharedWebSocketTest = require("../../test/js/joynr/messaging/websocket/SharedWebSocketTest");
let SubscriptionManagerTest = require("../../test/js/joynr/dispatching/subscription/SubscriptionManagerTest");
let SubscriptionPublicationTest = require("../../test/js/joynr/dispatching/types/SubscriptionPublicationTest");
let SubscriptionQosTest = require("../../test/js/joynr/proxy/SubscriptionQosTest");
let SubscriptionRequestTest = require("../../test/js/joynr/dispatching/types/SubscriptionRequestTest");
let SubscriptionUtilTest = require("../../test/js/joynr/dispatching/subscription/SubscriptionUtilTest");
let TtlUpliftTest = require("../../test/js/joynr/dispatching/TtlUpliftTest");
let TypeGeneratorTest = require("../../test/js/joynr/util/TypeGeneratorTest");
let TypingTest = require("../../test/js/joynr/util/TypingTest");
let UtilTest = require("../../test/js/joynr/util/UtilTest");
let WebMessagingAddressTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingAddressTest");
let WebMessagingSkeletonTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingSkeletonTest");
let WebMessagingStubFactoryTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingStubFactoryTest");
let WebMessagingStubTest = require("../../test/js/joynr/messaging/webmessaging/WebMessagingStubTest");
let WebSocketMessagingSkeletonTest = require("../../test/js/joynr/messaging/websocket/WebSocketMessagingSkeletonTest");
let WebSocketMessagingStubFactoryTest = require("../../test/js/joynr/messaging/websocket/WebSocketMessagingStubFactoryTest");
let WebSocketMessagingStubTest = require("../../test/js/joynr/messaging/websocket/WebSocketMessagingStubTest");
let WebSocketNodeTest = require("../../test/js/global/WebSocketNodeTest");
let WebSocketLibJoynrRuntimeTest = require("../../test/js/joynr/start/WebSocketLibjoynrRuntimeTest");
