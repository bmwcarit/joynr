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
const ChannelMessagingSender = require("../../../../../main/js/joynr/messaging/channel/ChannelMessagingSender");
const ChannelMessagingStubFactory = require("../../../../../main/js/joynr/messaging/channel/ChannelMessagingStubFactory");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingStubFactory", () => {
    let channelMessagingSender, channelMessagingStubFactory, destChannelId;
    let url, myChannelId, channelAddress1, channelAddress2, joynrMessage;

    beforeEach(done => {
        destChannelId = "destChannelId";
        myChannelId = "myChannelId";
        url = "http://testurl";
        channelMessagingSender = Object.create(ChannelMessagingSender.prototype);
        channelMessagingSender.send = jasmine.createSpy("channelMessagingSender.send");
        channelMessagingSender.send.and.returnValue(Promise.resolve());
        channelAddress1 = new ChannelAddress({
            channelId: destChannelId,
            messagingEndpointUrl: url
        });
        channelAddress2 = new ChannelAddress({
            channelId: myChannelId,
            messagingEndpointUrl: url
        });
        channelMessagingStubFactory = new ChannelMessagingStubFactory({
            channelMessagingSender
        });

        channelMessagingStubFactory.globalAddressReady(channelAddress2);
        joynrMessage = new JoynrMessage({
            key: "joynrMessage"
        });
        done();
    });

    it("is instantiable and of correct type", done => {
        expect(ChannelMessagingStubFactory).toBeDefined();
        expect(typeof ChannelMessagingStubFactory).toEqual("function");
        expect(channelMessagingStubFactory).toBeDefined();
        expect(channelMessagingStubFactory instanceof ChannelMessagingStubFactory).toEqual(true);
        expect(channelMessagingStubFactory.build).toBeDefined();
        expect(typeof channelMessagingStubFactory.build).toEqual("function");
        done();
    });

    it("creates a messaging stub and uses it correctly", done => {
        let channelMessagingStub = channelMessagingStubFactory.build(channelAddress1);
        channelMessagingStub.transmit(joynrMessage).catch(() => {
            return null;
        });
        expect(channelMessagingSender.send).toHaveBeenCalledWith(joynrMessage, channelAddress1);

        channelMessagingSender.send.calls.reset();
        //in case of target channelId is the local one --> missconfiguration, drop the message
        channelMessagingStub = channelMessagingStubFactory.build(channelAddress2);
        channelMessagingStub.transmit(joynrMessage).catch(() => {
            return null;
        });
        expect(channelMessagingSender.send).not.toHaveBeenCalled();
        done();
    });
});
