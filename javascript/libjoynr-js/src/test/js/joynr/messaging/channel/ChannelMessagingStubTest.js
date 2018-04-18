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
const Promise = require("../../../../../main/js/global/Promise");
const ChannelAddress = require("../../../../../main/js/generated/joynr/system/RoutingTypes/ChannelAddress");
const ChannelMessagingStub = require("../../../../../main/js/joynr/messaging/channel/ChannelMessagingStub");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingStub", () => {
    let channelMessagingSender, destinationChannelAddress, myChannelAddress;
    let channelMessagingStub1, channelMessagingStub2, joynrMessage;
    const url = "http://testurl";

    beforeEach(done => {
        channelMessagingSender = jasmine.createSpyObj("channelMessagingSender", ["send"]);
        channelMessagingSender.send.and.returnValue(Promise.resolve());
        destinationChannelAddress = new ChannelAddress({
            channelId: "destChannelId",
            messagingEndpointUrl: url
        });
        myChannelAddress = new ChannelAddress({
            channelId: "myChannelId",
            messagingEndpointUrl: url
        });

        channelMessagingStub1 = new ChannelMessagingStub({
            destinationChannelAddress,
            myChannelAddress,
            channelMessagingSender
        });
        channelMessagingStub2 = new ChannelMessagingStub({
            destinationChannelAddress,
            myChannelAddress: destinationChannelAddress,
            channelMessagingSender
        });
        joynrMessage = new JoynrMessage({
            key: "joynrMessage",
            type: "request"
        });
        done();
    });

    it("is instantiable and of correct type", done => {
        expect(ChannelMessagingStub).toBeDefined();
        expect(typeof ChannelMessagingStub).toEqual("function");
        expect(channelMessagingStub1).toBeDefined();
        expect(channelMessagingStub1 instanceof ChannelMessagingStub).toEqual(true);
        expect(channelMessagingStub1.transmit).toBeDefined();
        expect(typeof channelMessagingStub1.transmit).toEqual("function");
        done();
    });

    it("drop outgoing message if destChannel = myChannel", done => {
        channelMessagingStub2.transmit(joynrMessage).catch(() => {
            return null;
        });
        expect(channelMessagingSender.send).not.toHaveBeenCalled();
        expect(joynrMessage.replyChannelId).toBeUndefined();
        done();
    });

    it("transmits a message", done => {
        channelMessagingStub1.transmit(joynrMessage);
        expect(channelMessagingSender.send).toHaveBeenCalledWith(joynrMessage, destinationChannelAddress);
        done();
    });
});
