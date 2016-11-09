/*jslint es5: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
    "global/Promise",
    "joynr/system/RoutingTypes/ChannelAddress",
    "joynr/messaging/channel/ChannelMessagingStubFactory",
    "joynr/messaging/MessageReplyToAddressCalculator"
], function(Promise, ChannelAddress, ChannelMessagingStubFactory, MessageReplyToAddressCalculator) {

    describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingStubFactory", function() {
        var channelMessagingSender, channelMessagingStubFactory, destChannelId;
        var url, myChannelId, channelAddress1, channelAddress2, joynrMessage;

        beforeEach(function(done) {
            destChannelId = "destChannelId";
            myChannelId = "myChannelId";
            url = "http://testurl";
            channelMessagingSender = jasmine.createSpyObj("channelMessagingSender", [ "send"
            ]);
            channelMessagingSender.send.and.returnValue(Promise.resolve());
            channelAddress1 = new ChannelAddress({
                channelId : destChannelId,
                messagingEndpointUrl : url
            });
            channelAddress2 = new ChannelAddress({
                channelId : myChannelId,
                messagingEndpointUrl : url
            });
            channelMessagingStubFactory = new ChannelMessagingStubFactory({
                channelMessagingSender : channelMessagingSender,
                messageReplyToAddressCalculator : new MessageReplyToAddressCalculator({
                    replyToAddress : ""
                })
            });

            channelMessagingStubFactory.globalAddressReady(channelAddress2);
            joynrMessage = {
                key : "joynrMessage"
            };
            done();
        });

        it(
                "is instantiable and of correct type",
                function(done) {
                    expect(ChannelMessagingStubFactory).toBeDefined();
                    expect(typeof ChannelMessagingStubFactory === "function").toBeTruthy();
                    expect(channelMessagingStubFactory).toBeDefined();
                    expect(channelMessagingStubFactory instanceof ChannelMessagingStubFactory)
                            .toBeTruthy();
                    expect(channelMessagingStubFactory.build).toBeDefined();
                    expect(typeof channelMessagingStubFactory.build === "function").toBeTruthy();
                    done();
                });

        it("creates a messaging stub and uses it correctly", function(done) {
            var channelMessagingStub = channelMessagingStubFactory.build(channelAddress1);
            channelMessagingStub.transmit(joynrMessage).catch(function() { return null; });
            expect(channelMessagingSender.send).toHaveBeenCalledWith(
                joynrMessage,
                channelAddress1);

            channelMessagingSender.send.calls.reset();
            //in case of target channelId is the local one --> missconfiguration, drop the message
            channelMessagingStub = channelMessagingStubFactory.build(channelAddress2);
            channelMessagingStub.transmit(joynrMessage).catch(function() { return null; });
            expect(channelMessagingSender.send).not.toHaveBeenCalled();
            done();
        });

    });

});
