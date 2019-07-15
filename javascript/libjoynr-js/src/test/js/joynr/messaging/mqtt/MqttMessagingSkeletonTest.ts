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

import MqttMessagingSkeleton from "../../../../../main/js/joynr/messaging/mqtt/MqttMessagingSkeleton";
import JoynrMessage from "../../../../../main/js/joynr/messaging/JoynrMessage";
import MessageRouter from "../../../../../main/js/joynr/messaging/routing/MessageRouter";

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingSkeleton", () => {
    let sharedMqttClient: any, mqttMessagingSkeleton: MqttMessagingSkeleton;
    let messageRouterSpy: any;

    beforeEach(() => {
        messageRouterSpy = Object.create(MessageRouter.prototype);
        messageRouterSpy.route = jest.fn();
        messageRouterSpy.route.mockReturnValue(Promise.resolve());

        sharedMqttClient = { subscribe: jest.fn() };

        mqttMessagingSkeleton = new MqttMessagingSkeleton({
            address: { topic: "#kjg" } as any,
            client: sharedMqttClient,
            messageRouter: messageRouterSpy
        });

        sharedMqttClient.subscribe.mockClear();
    });

    function testCorrectMulticastIdTransformation(multicastId: any, expectedTopic: any) {
        mqttMessagingSkeleton.registerMulticastSubscription(multicastId);
        expect(sharedMqttClient.subscribe).toHaveBeenCalled();
        expect(sharedMqttClient.subscribe).toHaveBeenCalledWith(expectedTopic);
        sharedMqttClient.subscribe.mockClear();
    }

    it("correctly transform multicastIds to mqtt topics", () => {
        testCorrectMulticastIdTransformation("a/b", "a/b");
        testCorrectMulticastIdTransformation("a/+/b", "a/+/b");
        testCorrectMulticastIdTransformation("a/*", "a/#");
        testCorrectMulticastIdTransformation("x/y/z/*", "x/y/z/#");
    });

    function setsReceivedFromGlobal(message: any) {
        messageRouterSpy.route.mockClear();
        expect(message.isReceivedFromGlobal).toEqual(false);
        expect(messageRouterSpy.route).not.toHaveBeenCalled();
        sharedMqttClient.onmessage("", message);
        expect(messageRouterSpy.route).toHaveBeenCalledTimes(1);
        expect(messageRouterSpy.route.mock.calls[0][0].isReceivedFromGlobal).toEqual(true);
    }

    it("sets receivedFromGlobal", () => {
        const requestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "#mjga"
        });
        setsReceivedFromGlobal(requestMessage);

        const multicastMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
            payload: "#mjga"
        });
        setsReceivedFromGlobal(multicastMessage);

        const subscriptionRequestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload: "#mjga"
        });
        setsReceivedFromGlobal(subscriptionRequestMessage);
    });
});
