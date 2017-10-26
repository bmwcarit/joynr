/*jslint node: true */

/**
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
var MqttMessagingSkeleton = require("../../../../classes/joynr/messaging/mqtt/MqttMessagingSkeleton");
var JoynrMessage = require("../../../../classes/joynr/messaging/JoynrMessage");
var MessageRouter = require("../../../../classes/joynr/messaging/routing/MessageRouter");
var Promise = require("../../../../classes/global/Promise");

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingSkeleton", function() {
    var sharedMqttClient, mqttMessagingSkeleton;
    var messageRouterSpy;

    beforeEach(function() {
        function SharedMqttClient() {
            this.subscribe = function() {};
        }

        messageRouterSpy = Object.create(MessageRouter.prototype);
        messageRouterSpy.route = jasmine.createSpy("messageRouterSpy.route");
        messageRouterSpy.route.and.returnValue(Promise.resolve());

        function MqttAddress() {}

        sharedMqttClient = new SharedMqttClient();

        spyOn(sharedMqttClient, "subscribe");

        mqttMessagingSkeleton = new MqttMessagingSkeleton({
            address: new MqttAddress(),
            client: sharedMqttClient,
            messageRouter: messageRouterSpy
        });

        sharedMqttClient.subscribe.calls.reset();
    });

    function testCorrectMulticastIdTransformation(multicastId, expectedTopic) {
        mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
        expect(sharedMqttClient.subscribe).toHaveBeenCalled();
        expect(sharedMqttClient.subscribe).toHaveBeenCalledWith(expectedTopic);
        sharedMqttClient.subscribe.calls.reset();
    }

    it("correctly transform multicastIds to mqtt topics", function() {
        testCorrectMulticastIdTransformation("a/b", "a/b");
        testCorrectMulticastIdTransformation("a/+/b", "a/+/b");
        testCorrectMulticastIdTransformation("a/*", "a/#");
        testCorrectMulticastIdTransformation("x/y/z/*", "x/y/z/#");
    });

    function setsReceivedFromGlobal(message) {
        messageRouterSpy.route.calls.reset();
        expect(message.isReceivedFromGlobal).toEqual(false);
        expect(messageRouterSpy.route).not.toHaveBeenCalled();
        sharedMqttClient.onmessage("", message);
        expect(messageRouterSpy.route).toHaveBeenCalledTimes(1);
        expect(messageRouterSpy.route.calls.argsFor(0)[0].isReceivedFromGlobal).toEqual(true);
    }

    it("sets receivedFromGlobal", function() {
        var requestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        setsReceivedFromGlobal(requestMessage);

        var multicastMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
        });
        setsReceivedFromGlobal(multicastMessage);

        var subscriptionRequestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST
        });
        setsReceivedFromGlobal(subscriptionRequestMessage);
    });
});
