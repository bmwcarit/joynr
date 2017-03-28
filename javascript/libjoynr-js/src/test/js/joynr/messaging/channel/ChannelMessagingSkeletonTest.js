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
    "joynr/messaging/channel/ChannelMessagingSkeleton",
    "joynr/system/RoutingTypes/ChannelAddress"
], function(ChannelMessagingSkeleton, ChannelAddress) {

    describe("libjoynr-js.joynr.messaging.channel.ChannelMessagingSkeleton", function() {

        var channelMessagingSkeleton, joynrMessage1, joynrMessage2, messageRouterSpy;
        var channelAddress = new ChannelAddress({
            channelId : "channelId",
            messagingEndpointUrl : "http://testurl"
        });

        beforeEach(function(done) {
            messageRouterSpy = jasmine.createSpyObj("messageRouterSpy", [
                "addNextHop",
                "route"
            ]);
            channelMessagingSkeleton = new ChannelMessagingSkeleton({
                messageRouter : messageRouterSpy
            });

            joynrMessage1 = {
                key : "joynrMessage2"
            };
            joynrMessage2 = {
                key : "joynrMessage1",
                replyChannelId : JSON.stringify(channelAddress)
            };
            done();
        });

        it("is of correct type and has all members", function(done) {
            expect(ChannelMessagingSkeleton).toBeDefined();
            expect(typeof ChannelMessagingSkeleton === "function").toBeTruthy();
            expect(channelMessagingSkeleton).toBeDefined();
            expect(channelMessagingSkeleton instanceof ChannelMessagingSkeleton).toBeTruthy();
            done();
        });

        it("throws if arguments are missing or of wrong type", function(done) {
            expect(function() {
                channelMessagingSkeleton = new ChannelMessagingSkeleton();
            }).toThrow(); // correct call
            expect(function() {
                channelMessagingSkeleton = new ChannelMessagingSkeleton({
                    messageRouter : messageRouterSpy
                });
            }).not.toThrow(); // correct call
            done();
        });

        it(
                "event calls through to messageRouter",
                function(done) {
                    expect(messageRouterSpy.route).not.toHaveBeenCalled();
                    channelMessagingSkeleton.receiveMessage(joynrMessage1);
                    expect(messageRouterSpy.route).toHaveBeenCalledWith(joynrMessage1);
                    expect(messageRouterSpy.addNextHop).not.toHaveBeenCalled();
                    expect(messageRouterSpy.route.calls.count()).toBe(1);
                    channelMessagingSkeleton.receiveMessage(joynrMessage2);
                    expect(messageRouterSpy.route).toHaveBeenCalledWith(joynrMessage2);
                    expect(messageRouterSpy.addNextHop).toHaveBeenCalled();
                    expect(messageRouterSpy.addNextHop.calls.mostRecent().args[0]).toBe(
                            joynrMessage2.from);
                    expect(messageRouterSpy.addNextHop.calls.mostRecent().args[1].channelId).toBe(
                            channelAddress.channelId);
                    expect(messageRouterSpy.route.calls.count()).toBe(2);
                    done();
                });

    });

});
