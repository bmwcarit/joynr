/*jslint es5: true */

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

define([
    "joynr/messaging/JoynrMessage",
    "joynr/system/RoutingTypes/MqttAddress",
    "joynr/messaging/MessageReplyToAddressCalculator"
], function(JoynrMessage, MqttAddress, MessageReplyToAddressCalculator) {

    describe("libjoynr-js.joynr.messaging.MessageReplyToAddressCalculator", function() {
        var messageReplyToAddressCalculator;
        var globalAddress, serializedGlobalAddress;

        beforeEach(function() {
            messageReplyToAddressCalculator = new MessageReplyToAddressCalculator({});
            globalAddress = new MqttAddress({
                brokerUri : "testBrokerUri",
                topic : "testMqttTopic"
            });
            serializedGlobalAddress = JSON.stringify(globalAddress);
        });

        it("setReplyTo throws if replyToAddress not specified", function() {
            var request = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            expect(messageReplyToAddressCalculator.setReplyTo.bind(this, request)).toThrowError(
                    Error);
        });

        function testSetsReplyToAddressOfMessage(msg) {
            messageReplyToAddressCalculator.setReplyToAddress(globalAddress);
            expect(msg.replyChannelId).toEqual(undefined);
            messageReplyToAddressCalculator.setReplyTo(msg);
            expect(msg.replyChannelId).toEqual(serializedGlobalAddress);
        }

        it("sets replyTo address of request messages", function() {
            var request = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            testSetsReplyToAddressOfMessage(request);

            var subscriptionRequest = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST
            });
            testSetsReplyToAddressOfMessage(subscriptionRequest);

            var broadcastSubscriptionRequest = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST
            });
            testSetsReplyToAddressOfMessage(broadcastSubscriptionRequest);

            var multicastSubscriptionRequest = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST
            });
            testSetsReplyToAddressOfMessage(multicastSubscriptionRequest);
        });

        it("does not overwrite already set replyTo address", function() {
            messageReplyToAddressCalculator.setReplyToAddress(globalAddress);
            var request = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            var anotherReplyToAddress = "anotherReplyToAddress";
            request.replyChannelId = "anotherReplyToAddress";

            messageReplyToAddressCalculator.setReplyTo(request);
            expect(request.replyChannelId).toEqual(anotherReplyToAddress);
        });

        function testDoesNotSetReplyToAddressOfMessage(msg) {
            messageReplyToAddressCalculator.setReplyToAddress(globalAddress);
            expect(msg.replyChannelId).toEqual(undefined);
            messageReplyToAddressCalculator.setReplyTo(msg);
            expect(msg.replyChannelId).toEqual(undefined);
        }

        it("does not set replyTo address of other message types", function() {
            var msg = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY
            });
            testDoesNotSetReplyToAddressOfMessage(msg);

            msg.type = JoynrMessage.JOYNRMESSAGE_TYPE_REPLY;
            testDoesNotSetReplyToAddressOfMessage(msg);

            msg.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY;
            testDoesNotSetReplyToAddressOfMessage(msg);

            msg.type = JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION;
            testDoesNotSetReplyToAddressOfMessage(msg);

            msg.type = JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST;
            testDoesNotSetReplyToAddressOfMessage(msg);

            msg.type = JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP;
            testDoesNotSetReplyToAddressOfMessage(msg);
        });

    }); // describe MessageReplyToAddressCalculator
}); // define
