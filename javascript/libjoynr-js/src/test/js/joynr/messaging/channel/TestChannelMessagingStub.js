/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

joynrTestRequire("joynr/messaging/channel/TestChannelMessagingStub", [
    "global/Promise",
    "joynr/messaging/channel/ChannelMessagingStub"
], function(Promise, ChannelMessagingStub) {

    describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingStub", function() {
        var channelMessagingSender, destChannelId, myChannelId;
        var channelMessagingStub1, channelMessagingStub2, joynrMessage;

        beforeEach(function() {
            channelMessagingSender = jasmine.createSpyObj("channelMessagingSender", [ "send"
            ]);
            channelMessagingSender.send.andReturn(Promise.resolve());
            destChannelId = "destChannelId";
            myChannelId = "myChannelId";
            channelMessagingStub1 = new ChannelMessagingStub({
                destChannelId : destChannelId,
                myChannelId : myChannelId,
                channelMessagingSender : channelMessagingSender
            });
            channelMessagingStub2 = new ChannelMessagingStub({
                destChannelId : destChannelId,
                myChannelId : destChannelId,
                channelMessagingSender : channelMessagingSender
            });
            joynrMessage = {
                key : "joynrMessage"
            };
        });

        it("is instantiable and of correct type", function() {
            expect(ChannelMessagingStub).toBeDefined();
            expect(typeof ChannelMessagingStub === "function").toBeTruthy();
            expect(channelMessagingStub1).toBeDefined();
            expect(channelMessagingStub1 instanceof ChannelMessagingStub).toBeTruthy();
            expect(channelMessagingStub1.transmit).toBeDefined();
            expect(typeof channelMessagingStub1.transmit === "function").toBeTruthy();
        });

        it("drop outgoing message if destChannel = myChannel", function() {
            channelMessagingStub2.transmit(joynrMessage);
            expect(channelMessagingSender.send).not.toHaveBeenCalled();
            expect(joynrMessage.replyChannelId).toBeUndefined();
        });

        it("transmits a message and set replyChannelId", function() {
            expect(joynrMessage.replyChannelId).toBeUndefined();
            var result = channelMessagingStub1.transmit(joynrMessage);
            expect(channelMessagingSender.send).toHaveBeenCalledWith(joynrMessage, destChannelId);
            expect(joynrMessage.replyChannelId).toBe(myChannelId);
        });

    });

});