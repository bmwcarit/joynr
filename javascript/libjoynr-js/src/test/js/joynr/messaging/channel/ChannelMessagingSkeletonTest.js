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
require("../../../node-unit-test-helper");
const ChannelMessagingSkeleton = require("../../../../../main/js/joynr/messaging/channel/ChannelMessagingSkeleton");
const ChannelAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/ChannelAddress");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
const Promise = require("../../../../../main/js/global/Promise");

describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingSkeleton", () => {
    let channelMessagingSkeleton, joynrMessage1, joynrMessage2, messageRouterSpy;
    const channelAddress = new ChannelAddress({
        channelId: "channelId",
        messagingEndpointUrl: "http://testurl"
    });

    beforeEach(done => {
        messageRouterSpy = jasmine.createSpyObj("messageRouterSpy", ["addNextHop", "route"]);
        messageRouterSpy.route.and.returnValue(Promise.resolve());
        channelMessagingSkeleton = new ChannelMessagingSkeleton({
            messageRouter: messageRouterSpy
        });

        joynrMessage1 = {
            key: "joynrMessage2"
        };
        joynrMessage2 = {
            key: "joynrMessage1",
            replyChannelId: JSON.stringify(channelAddress) // TODO: check why replyChannelId is not in header
        };
        done();
    });

    it("is of correct type and has all members", done => {
        expect(ChannelMessagingSkeleton).toBeDefined();
        expect(typeof ChannelMessagingSkeleton === "function").toBeTruthy();
        expect(channelMessagingSkeleton).toBeDefined();
        expect(channelMessagingSkeleton instanceof ChannelMessagingSkeleton).toBeTruthy();
        done();
    });

    it("throws if arguments are missing or of wrong type", done => {
        expect(() => {
            channelMessagingSkeleton = new ChannelMessagingSkeleton();
        }).toThrow(); // correct call
        expect(() => {
            channelMessagingSkeleton = new ChannelMessagingSkeleton({
                messageRouter: messageRouterSpy
            });
        }).not.toThrow(); // correct call
        done();
    });

    it("event calls through to messageRouter", done => {
        expect(messageRouterSpy.route).not.toHaveBeenCalled();

        channelMessagingSkeleton.receiveMessage(joynrMessage1);
        expect(messageRouterSpy.route).toHaveBeenCalledWith(joynrMessage1);
        expect(messageRouterSpy.route.calls.count()).toBe(1);

        channelMessagingSkeleton.receiveMessage(joynrMessage2);
        expect(messageRouterSpy.route).toHaveBeenCalledWith(joynrMessage2);
        expect(messageRouterSpy.route.calls.count()).toBe(2);

        expect(messageRouterSpy.addNextHop).not.toHaveBeenCalled();
        done();
    });

    function setsReceivedFromGlobal(message) {
        messageRouterSpy.route.calls.reset();
        expect(message.isReceivedFromGlobal).toEqual(false);
        expect(messageRouterSpy.route).not.toHaveBeenCalled();
        channelMessagingSkeleton.receiveMessage(message);
        expect(messageRouterSpy.route).toHaveBeenCalledTimes(1);
        expect(messageRouterSpy.route.calls.argsFor(0)[0].isReceivedFromGlobal).toEqual(true);
    }

    it("sets receivedFromGlobal", () => {
        const requestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        setsReceivedFromGlobal(requestMessage);

        const multicastMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
        });
        setsReceivedFromGlobal(multicastMessage);

        const subscriptionRequestMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST
        });
        setsReceivedFromGlobal(subscriptionRequestMessage);
    });
});
