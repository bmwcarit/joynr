/*jslint es5: true, node: true, node: true */
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
var Promise = require('../../../../classes/global/Promise');
var ChannelAddress = require('../../../../classes/joynr/system/RoutingTypes/ChannelAddress');
var ChannelMessagingStub = require('../../../../classes/joynr/messaging/channel/ChannelMessagingStub');

    describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingStub", function() {
        var channelMessagingSender, destinationChannelAddress, myChannelAddress;
        var channelMessagingStub1, channelMessagingStub2, joynrMessage;
        var url = "http://testurl";

        beforeEach(function(done) {
            channelMessagingSender = jasmine.createSpyObj("channelMessagingSender", [ "send"
            ]);
            channelMessagingSender.send.and.returnValue(Promise.resolve());
            destinationChannelAddress = new ChannelAddress({
                channelId : "destChannelId",
                messagingEndpointUrl : url
            });
            myChannelAddress = new ChannelAddress({
                channelId : "myChannelId",
                messagingEndpointUrl : url
            });

            channelMessagingStub1 = new ChannelMessagingStub({
                destinationChannelAddress : destinationChannelAddress,
                myChannelAddress : myChannelAddress,
                channelMessagingSender : channelMessagingSender
            });
            channelMessagingStub2 = new ChannelMessagingStub({
                destinationChannelAddress : destinationChannelAddress,
                myChannelAddress : destinationChannelAddress,
                channelMessagingSender : channelMessagingSender
            });
            joynrMessage = {
                key : "joynrMessage",
                type : "request"
            };
            done();
        });

        it("is instantiable and of correct type", function(done) {
            expect(ChannelMessagingStub).toBeDefined();
            expect(typeof ChannelMessagingStub).toEqual("function");
            expect(channelMessagingStub1).toBeDefined();
            expect(channelMessagingStub1 instanceof ChannelMessagingStub).toEqual(true);
            expect(channelMessagingStub1.transmit).toBeDefined();
            expect(typeof channelMessagingStub1.transmit).toEqual("function");
            done();
        });

        it("drop outgoing message if destChannel = myChannel", function(done) {
            channelMessagingStub2.transmit(joynrMessage).catch(function() { return null; });
            expect(channelMessagingSender.send).not.toHaveBeenCalled();
            expect(joynrMessage.replyChannelId).toBeUndefined();
            done();
        });

        it("transmits a message", function(done) {
            channelMessagingStub1.transmit(joynrMessage);
            expect(channelMessagingSender.send).toHaveBeenCalledWith(
                    joynrMessage,
                    destinationChannelAddress);
            done();
        });

    });
