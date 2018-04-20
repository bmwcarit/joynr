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
    "global/Promise",
    "joynr/util/UtilInternal",
    "joynr/messaging/CommunicationModule",
    "joynr/messaging/channel/LongPollingChannelMessageReceiver",
    "joynr/messaging/JoynrMessage",
    "joynr/messaging/channel/ChannelMessagingSender",
    "joynr/system/RoutingTypes/ChannelAddress",
    "joynr/system/LoggerFactory",
    "global/LocalStorage",
    "joynr/provisioning/provisioning_cc",
    "global/WaitsFor"
], function(
        Promise,
        Util,
        CommunicationModule,
        LongPollingChannelMessageReceiver,
        JoynrMessage,
        ChannelMessagingSender,
        ChannelAddress,
        LoggerFactory,
        LocalStorage,
        provisioning,
        waitsFor) {
    var log = LoggerFactory.getLogger("joynr.messaging.HttpMessagingTest");
    var localStorage = new LocalStorage();

    describe("libjoynr-js.joynr.messaging.HttpMessagingTest", function() {
        it("sends and receives messages", function(done) {
            var channelId = "js_testOpenChannelSendMessage" + Date.now();
            var url = provisioning.bounceProxyUrl + "channels/" + channelId + "/";
            var channelAddress = new ChannelAddress({
                channelId : channelId,
                messagingEndpointUrl : url
            });

            var communicationModule = new CommunicationModule();

            var mr = new LongPollingChannelMessageReceiver({
                persistency : localStorage,
                bounceProxyUrl : provisioning.bounceProxyUrl,
                communicationModule : communicationModule
            });
            var numberOfMessages = 10;

            /*
             * Set up a JoynrMessage to send
             */
            var receivedMessages = 0;
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            joynrMessage.expiryDate = 9360686108031;
            joynrMessage.replyChannelId ="me";
            joynrMessage.payload = "hello";

            mr.create(channelId).then(function() {
                mr.start(function() {
                    ++receivedMessages;
                });
                var messageSender = new ChannelMessagingSender({
                    communicationModule : communicationModule
                });

                messageSender.start();
                var msgPromises = [];
                var i;
                for (i = 0; i < numberOfMessages; i++) {
                    msgPromises.push(messageSender.send(joynrMessage, channelAddress));
                }

                return Promise.all(msgPromises);
            }).then(function() {
                return waitsFor(function() {
                    return receivedMessages === numberOfMessages;
                }, "all messages are received", provisioning.ttl);
            }).then(function() {
                expect(receivedMessages).toBe(numberOfMessages);
                return mr.clear();
            }).then(function() {
                mr.stop();
                done();
                return null;
            }).catch(fail);
        }, provisioning.ttl * 5);
    });
});
