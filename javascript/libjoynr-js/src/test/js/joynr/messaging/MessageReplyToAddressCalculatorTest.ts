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

import JoynrMessage from "../../../../main/js/joynr/messaging/JoynrMessage";
import MqttAddress from "../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress";
import MessageReplyToAddressCalculator from "../../../../main/js/joynr/messaging/MessageReplyToAddressCalculator";

describe("libjoynr-js.joynr.messaging.MessageReplyToAddressCalculator", () => {
    let messageReplyToAddressCalculator: any;
    let globalAddress: any, serializedGlobalAddress: any;

    beforeEach(() => {
        messageReplyToAddressCalculator = new MessageReplyToAddressCalculator({});
        globalAddress = new MqttAddress({
            brokerUri: "testBrokerUri",
            topic: "testMqttTopic"
        });
        serializedGlobalAddress = JSON.stringify(globalAddress);
    });

    it("setReplyTo throws if replyToAddress not specified", () => {
        const request = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: ""
        });
        expect(() => messageReplyToAddressCalculator.setReplyTo(request)).toThrow(Error);
    });

    function testSetsReplyToAddressOfMessage(msg: any) {
        messageReplyToAddressCalculator.setReplyToAddress(JSON.stringify(globalAddress));
        expect(msg.replyChannelId).toEqual(undefined);
        messageReplyToAddressCalculator.setReplyTo(msg);
        expect(msg.replyChannelId).toEqual(serializedGlobalAddress);
    }

    it("sets replyTo address of request messages", () => {
        const request = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: ""
        });
        testSetsReplyToAddressOfMessage(request);

        const subscriptionRequest = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload: ""
        });
        testSetsReplyToAddressOfMessage(subscriptionRequest);

        const broadcastSubscriptionRequest = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST,
            payload: ""
        });
        testSetsReplyToAddressOfMessage(broadcastSubscriptionRequest);

        const multicastSubscriptionRequest = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
            payload: ""
        });
        testSetsReplyToAddressOfMessage(multicastSubscriptionRequest);
    });

    it("does not overwrite already set replyTo address", () => {
        messageReplyToAddressCalculator.setReplyToAddress(JSON.stringify(globalAddress));
        const request = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: ""
        });
        const anotherReplyToAddress = "anotherReplyToAddress";
        request.replyChannelId = "anotherReplyToAddress";

        messageReplyToAddressCalculator.setReplyTo(request);
        expect(request.replyChannelId).toEqual(anotherReplyToAddress);
    });

    function testDoesNotSetReplyToAddressOfMessage(msg: any) {
        messageReplyToAddressCalculator.setReplyToAddress(JSON.stringify(globalAddress));
        expect(msg.replyChannelId).toEqual(undefined);
        messageReplyToAddressCalculator.setReplyTo(msg);
        expect(msg.replyChannelId).toEqual(undefined);
    }

    it("does not set replyTo address of other message types", () => {
        const msg = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
            payload: ""
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
