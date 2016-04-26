/*global joynrTestRequire: true */
/*jslint es5: true */

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

joynrTestRequire(
        "joynr/messaging/channel/TestChannelMessagingSender",
        [
            "global/Promise",
            "joynr/messaging/channel/ChannelMessagingSender",
            "joynr/messaging/JoynrMessage",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/util/Typing",
            "joynr/system/LoggerFactory",
            "joynr/provisioning/provisioning_root"
        ],
        function(Promise, ChannelMessagingSender, JoynrMessage, ChannelAddress, Typing, LoggerFactory, provisioning) {

            var log = LoggerFactory.getLogger("joynr.messaging.TestChannelMessagingSender");

            describe(
                    "libjoynr-js.joynr.messaging.ChannelMessagingSender",
                    function() {
                        var communicationModuleSpy, channelMessageSender;
                        var channelId, channelUrl, channelUrlInformation, joynrMessage;
                        var resendDelay_ms;

                        function outputPromiseError(error) {
                            expect(error.toString()).toBeFalsy();
                        }

                        beforeEach(function() {
                            resendDelay_ms = 500;
                            channelId = "myChannel" + Date.now();
                            channelUrl = provisioning.bounceProxyUrl + "/channels/" + channelId;
                            var channelQos = {
                                resendDelay_ms : resendDelay_ms
                            };

                            joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                                    9360686108031);
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                                    "me");
                            joynrMessage.payload = "hello";

                            // instantiate spies
                            communicationModuleSpy =
                                    jasmine.createSpyObj(
                                            "communicationModule",
                                            [ "createXMLHTTPRequest"
                                            ]);

                            channelMessageSender = new ChannelMessagingSender({
                                communicationModule : communicationModuleSpy,
                                channelQos : channelQos
                            });
                        });

                        it("is instantiable and has all members", function() {
                            expect(ChannelMessagingSender).toBeDefined();
                            expect(typeof ChannelMessagingSender === "function").toBeTruthy();
                            expect(channelMessageSender).toBeDefined();
                            expect(channelMessageSender instanceof ChannelMessagingSender)
                                    .toBeTruthy();
                            expect(channelMessageSender.send).toBeDefined();
                            expect(typeof channelMessageSender.send === "function").toBeTruthy();
                        });

                        it(
                                "if communicationModule.createXMLHTTPRequest call fails, channelMessageSender only fails if message expires",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    var timeStamp;
                                    var relativeExpiryDate = resendDelay_ms * 3;

                                    runs(function() {
                                        communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.reject({
                                                    status : 500,
                                                    responseText : "responseText",
                                                    statusText : "errorThrown"
                                                }, "errorStatus"));
                                        joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] = Date.now() + relativeExpiryDate;
                                        channelMessageSender.start();
                                        timeStamp = Date.now();
                                        channelMessageSender.send(joynrMessage, new ChannelAddress({channelId: channelId, messagingEndpointUrl: "http://testurl"})).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "message send to fail", relativeExpiryDate * 5);

                                    runs(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(Date.now()-timeStamp>relativeExpiryDate).toEqual(true);
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                        expect(communicationModuleSpy.createXMLHTTPRequest)
                                                .toHaveBeenCalled();
                                    });
                                });

                        it(
                                "if channelMessageSender.send fails after expiryDate, if ChannelMessagingSender is not started",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    var relativeExpiryDate = resendDelay_ms;

                                    runs(function() {
                                        joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] = Date.now() + relativeExpiryDate;
                                        channelMessageSender.send(joynrMessage, channelId).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0 || spy.onRejected.callCount > 0;
                                    }, "message send to fail", relativeExpiryDate * 5);

                                    runs(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE] < Date.now()).toEqual(true);
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();

                                        spy.onRejected.reset();
                                        expect(communicationModuleSpy.createXMLHTTPRequest).not.toHaveBeenCalled();
                                    });
                                });

                        it(
                                "sends message using communicationModule.createXMLHTTPRequest",
                                function() {
                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    runs(function() {
                                        communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.resolve());
                                        channelMessageSender.start();
                                        channelMessageSender.send(joynrMessage, channelId).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onFulfilled.callCount > 0;
                                    }, "message send to fail", provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onFulfilled).toHaveBeenCalled();
                                        expect(spy.onFulfilled).toHaveBeenCalledWith(undefined);
                                        expect(spy.onRejected).not.toHaveBeenCalled();
                                        expect(communicationModuleSpy.createXMLHTTPRequest)
                                                .toHaveBeenCalled();
                                    });
                                });

                    });

        });