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

joynrTestRequire("integration/TestHttpMessaging", [
    "global/Promise",
    "joynr/util/UtilInternal",
    "joynr/messaging/CommunicationModule",
    "joynr/messaging/channel/LongPollingChannelMessageReceiver",
    "joynr/messaging/JoynrMessage",
    "joynr/types/ChannelUrlInformation",
    "joynr/messaging/channel/ChannelMessagingSender",
    "joynr/system/LoggerFactory",
    "global/LocalStorage",
    "joynr/provisioning/provisioning_cc"
], function(
        Promise,
        Util,
        CommunicationModule,
        LongPollingChannelMessageReceiver,
        JoynrMessage,
        ChannelUrlInformation,
        ChannelMessagingSender,
        LoggerFactory,
        LocalStorage,
        provisioning) {
    var log = LoggerFactory.getLogger("joynr.messaging.TestHttpMessaging");
    var localStorage = new LocalStorage();

    describe("libjoynr-js.joynr.messaging.TestHttpMessaging", function() {
        it(
                "sends and receives messages",
                function() {
                    var channel = "js_testOpenChannelSendMessage" + Date.now();
                    var url = provisioning.bounceProxyUrl + "channels/" + channel + "/";
                    var channelUrlInformation = new ChannelUrlInformation({
                        urls : [ url
                        ]
                    });
                    var channelUrlDirectory =
                            jasmine.createSpyObj("channelUrlDirectory", [ "getUrlsForChannel"
                            ]);
                    channelUrlDirectory.getUrlsForChannel.andReturn(Promise
                            .resolve(channelUrlInformation));

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
                    var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                    joynrMessage.setHeader(
                            JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                            9360686108031);
                    joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID, "me");
                    joynrMessage.payload = "hello";

                    var fulfilledSpy = jasmine.createSpy("fulfilledSpy");

                    runs(function() {
                        mr.create(channel).then(fulfilledSpy);
                    });

                    waitsFor(function() {
                        return fulfilledSpy.callCount > 0;
                    }, "channel to be created", provisioning.ttl);

                    var receivedMessages = 0;
                    runs(function() {
                        expect(fulfilledSpy).toHaveBeenCalled();

                        mr.start(function() {
                            ++receivedMessages;
                        });
                        var messageSender = new ChannelMessagingSender({
                            channelUrlDirectory : channelUrlDirectory,
                            communicationModule : communicationModule
                        });

                        messageSender.start();
                        var msgPromises = [];
                        var i;
                        for (i = 0; i < numberOfMessages; i++) {
                            msgPromises.push(messageSender.send(joynrMessage, channel));
                        }

                        fulfilledSpy.reset();
                        Promise.all(msgPromises).then(fulfilledSpy);
                    });

                    waitsFor(function() {
                        return fulfilledSpy.callCount > 0;
                    }, "all messages to be sent", provisioning.ttl);

                    waitsFor(function() {
                        return receivedMessages === numberOfMessages;
                    }, "all messages are received", provisioning.ttl);

                    runs(function() {
                        expect(fulfilledSpy).toHaveBeenCalled();
                        expect(receivedMessages).toBe(numberOfMessages);

                        fulfilledSpy.reset();

                        mr.clear(channel).then(fulfilledSpy);
                    });

                    waitsFor(function() {
                        return fulfilledSpy.callCount > 0;
                    }, "channel to be removed", provisioning.ttl);

                    runs(function() {
                        expect(fulfilledSpy).toHaveBeenCalled();

                        mr.stop();
                    });
                });
    });

});