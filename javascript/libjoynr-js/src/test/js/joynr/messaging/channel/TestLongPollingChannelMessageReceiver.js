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
        "joynr/messaging/channel/TestLongPollingChannelMessageReceiver",
        [
            "global/Promise",
            "joynr/messaging/CommunicationModule",
            "joynr/messaging/channel/LongPollingChannelMessageReceiver",
            "joynr/messaging/JoynrMessage",
            "joynr/util/Typing",
            "joynr/system/LoggerFactory",
            "global/LocalStorage",
            "joynr/provisioning/provisioning_root"
        ],
        function(
                Promise,
                CommunicationModule,
                LongPollingChannelMessageReceiver,
                JoynrMessage,
                Typing,
                LoggerFactory,
                LocalStorage,
                provisioning) {

            var log =
                    LoggerFactory
                            .getLogger("joynr.messaging.TestLongPollingChannelMessageReceiver");
            var localStorage = new LocalStorage();

            describe(
                    "libjoynr-js.joynr.messaging.LongPollingChannelMessageReceiver",
                    function() {
                        var atmosphereSpy, communicationModuleSpy, longPollingChannelMessageReceiver;
                        var channelId, channelUrl, joynrMessage;
                        var channelCreationTimeout_ms, channelCreationRetryDelay_ms;

                        function outputPromiseError(error) {
                            expect(error.toString()).toBeFalsy();
                        }

                        function createChannel() {
                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);

                            runs(function() {
                                communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.resolve({
                                            getResponseHeader : function() {
                                                return channelUrl;
                                            }
                                        }));
                                longPollingChannelMessageReceiver.create(channelId).then(
                                        spy.onFulfilled).catch(spy.onRejected).catch(outputPromiseError);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "channel is created", provisioning.ttl);

                            return spy;
                        }

                        beforeEach(function() {
                            channelId = "myChannel" + Date.now();
                            channelUrl = provisioning.bounceProxyUrl + "/channels/" + channelId;
                            channelCreationTimeout_ms = 1000;
                            channelCreationRetryDelay_ms = 50;
                            var channelQos = {
                                creationTimeout_ms: channelCreationTimeout_ms,
                                creationRetryDelay_ms: channelCreationRetryDelay_ms
                            };

                            joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE,
                                    9360686108031);
                            joynrMessage.setHeader(
                                    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                                    "me");
                            joynrMessage.payload = "hello";

                            // instantiate CommunicationModule spy
                            atmosphereSpy = jasmine.createSpyObj("atmosphere", [
                                "subscribe",
                                "unsubscribeUrl"
                            ]);
                            communicationModuleSpy =
                                    jasmine.createSpyObj(
                                            "communicationModule",
                                            [ "createXMLHTTPRequest"
                                            ]);
                            communicationModuleSpy.atmosphere = atmosphereSpy;

                            longPollingChannelMessageReceiver =
                                    new LongPollingChannelMessageReceiver({
                                        persistency : localStorage,
                                        bounceProxyUrl : provisioning.bounceProxyUrl,
                                        communicationModule : communicationModuleSpy,
                                        channelQos : channelQos
                                    });
                        });

                        it("create fails only if channelCreationTimeout_ms exceed", function(){
                            var spy = jasmine.createSpyObj("spy", [
                                                                   "onFulfilled",
                                                                   "onRejected"
                                                               ]);
                            var createChannelCallTimestamp;

                            runs(function(){
                                communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.reject(new Error("fake http request failed")));
                                createChannelCallTimestamp = Date.now();
                                longPollingChannelMessageReceiver.create(channelId).then(spy.onFulfilled).catch(spy.onRejected).catch(outputPromiseError);
                            });

                            waitsFor(function(){
                                return spy.onRejected.callCount > 0;
                            }, "createChannel failed", channelCreationTimeout_ms + 1000);

                            runs(function(){
                                expect(Date.now() - createChannelCallTimestamp > channelCreationTimeout_ms).toEqual(true);
                            });
                        });

                        it(
                                "is instantiable and has all members",
                                function() {
                                    expect(longPollingChannelMessageReceiver).toBeDefined();
                                    expect(
                                            longPollingChannelMessageReceiver instanceof LongPollingChannelMessageReceiver)
                                            .toBeTruthy();
                                    expect(longPollingChannelMessageReceiver.create).toBeDefined();
                                    expect(longPollingChannelMessageReceiver.start).toBeDefined();
                                    expect(longPollingChannelMessageReceiver.clear).toBeDefined();
                                    expect(longPollingChannelMessageReceiver.stop).toBeDefined();
                                    expect(
                                            typeof longPollingChannelMessageReceiver.create === "function")
                                            .toBeTruthy();
                                    expect(
                                            typeof longPollingChannelMessageReceiver.start === "function")
                                            .toBeTruthy();
                                    expect(
                                            typeof longPollingChannelMessageReceiver.clear === "function")
                                            .toBeTruthy();
                                    expect(
                                            typeof longPollingChannelMessageReceiver.stop === "function")
                                            .toBeTruthy();
                                });

                        it("create is successful if channel is created", function() {
                            var spy = createChannel();

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith(channelUrl);
                                expect(spy.onRejected).not.toHaveBeenCalled();
                                expect(communicationModuleSpy.createXMLHTTPRequest)
                                        .toHaveBeenCalled();
                                expect(communicationModuleSpy.createXMLHTTPRequest)
                                        .toHaveBeenCalledWith(jasmine.any(Object));
                            });
                        });

                        it(
                                "create is unsuccessful if channel is not created",
                                function() {
                                    var spy;
                                    runs(function() {
                                        communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.reject({
                                                    status : 500,
                                                    responseText : "responseText"
                                                }, "errorThrown"));
                                        spy = jasmine.createSpyObj("spy", [
                                            "onFulfilled",
                                            "onRejected"
                                        ]);
                                        longPollingChannelMessageReceiver.create(channelId).then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "channel is created", provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                    });
                                });

                        it("after a call to start, messages are transmitted", function() {
                            var messageSpy = jasmine.createSpy("messageSpy");
                            longPollingChannelMessageReceiver.start(messageSpy);
                            expect(atmosphereSpy.subscribe).toHaveBeenCalled();
                            expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalled();

                            var messageCallback =
                                    atmosphereSpy.subscribe.calls[0].args[0].onMessage;
                            expect(messageSpy).not.toHaveBeenCalled();
                            messageCallback({
                                status : 200,
                                responseBody : JSON.stringify(joynrMessage)
                            });
                            expect(messageSpy).toHaveBeenCalled();
                            // Doesn't work because, probably because of JoynrMessage magic: expect(messageSpy).toHaveBeenCalledWith(joynrMessage);
                            expect(JSON.stringify(messageSpy.calls[0].args[0])).toEqual(
                                    JSON.stringify(joynrMessage));
                        });

                        it("clear is successful if channel is cleared", function() {
                            createChannel();

                            var spy = jasmine.createSpyObj("spy", [
                                "onFulfilled",
                                "onRejected"
                            ]);
                            runs(function() {
                                communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.resolve({
                                            getResponseHeader : function() {
                                                return channelUrl;
                                            }
                                        }));
                                longPollingChannelMessageReceiver.clear().then(
                                        spy.onFulfilled).catch(spy.onRejected).catch(outputPromiseError);
                            });

                            waitsFor(function() {
                                return spy.onFulfilled.callCount > 0;
                            }, "channel is cleared", provisioning.ttl);

                            runs(function() {
                                expect(spy.onFulfilled).toHaveBeenCalled();
                                expect(spy.onFulfilled).toHaveBeenCalledWith(undefined);
                                expect(spy.onRejected).not.toHaveBeenCalled();
                            });
                        });

                        it(
                                "clear is unsuccessful if channel is not cleared",
                                function() {
                                    createChannel();

                                    var spy = jasmine.createSpyObj("spy", [
                                        "onFulfilled",
                                        "onRejected"
                                    ]);
                                    runs(function() {
                                        communicationModuleSpy.createXMLHTTPRequest.andReturn(Promise.reject({
                                                    status : 500,
                                                    responseText : "responseText"
                                                }, "errorThrown"));
                                        longPollingChannelMessageReceiver.clear().then(
                                                spy.onFulfilled).catch(spy.onRejected);
                                    });

                                    waitsFor(function() {
                                        return spy.onRejected.callCount > 0;
                                    }, "channel is cleared", provisioning.ttl);

                                    runs(function() {
                                        expect(spy.onFulfilled).not.toHaveBeenCalled();
                                        expect(spy.onRejected).toHaveBeenCalled();
                                        expect(
                                                Object.prototype.toString
                                                        .call(spy.onRejected.mostRecentCall.args[0]) === "[object Error]")
                                                .toBeTruthy();
                                    });
                                });

                        it("stops", function() {
                            createChannel();

                            runs(function(){
                                longPollingChannelMessageReceiver.stop();
                                expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalled();
                                expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalledWith(channelUrl);
                            });
                        });

                    });

        });