/*jslint es5: true, node: true, node: true */
/*global fail: true */
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
var Promise = require("../../../../classes/global/Promise");
var CommunicationModule = require("../../../../classes/joynr/messaging/CommunicationModule");
var LongPollingChannelMessageReceiver = require("../../../../classes/joynr/messaging/channel/LongPollingChannelMessageReceiver");
var JoynrMessage = require("../../../../classes/joynr/messaging/JoynrMessage");
var Typing = require("../../../../classes/joynr/util/Typing");
var LoggerFactory = require("../../../../classes/joynr/system/LoggerFactory");
var LocalStorage = require("../../../../test-classes/global/LocalStorageNodeTests");
var provisioning = require("../../../../test-classes/joynr/provisioning/provisioning_root");
var waitsFor = require("../../../../test-classes/global/WaitsFor");

var log = LoggerFactory.getLogger("joynr.messaging.TestLongPollingChannelMessageReceiver");
var localStorage = new LocalStorage();

describe("libjoynr-js.joynr.messaging.LongPollingChannelMessageReceiver", function() {
    var atmosphereSpy, communicationModuleSpy, longPollingChannelMessageReceiver;
    var channelId, channelUrl, joynrMessage;
    var channelCreationTimeout_ms, channelCreationRetryDelay_ms;

    function outputPromiseError(error) {
        expect(error.toString()).toBeFalsy();
    }

    function createChannel() {
        var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
            Promise.resolve({
                getResponseHeader: function() {
                    return channelUrl;
                }
            })
        );
        longPollingChannelMessageReceiver
            .create(channelId)
            .then(spy.onFulfilled)
            .catch(spy.onRejected)
            .catch(outputPromiseError);

        var returnValue = waitsFor(
            function() {
                return spy.onFulfilled.calls.count() > 0;
            },
            "channel is created",
            provisioning.ttl
        ).then(function() {
            return new Promise(function(resolve, reject) {
                resolve(spy);
            });
        });
        return returnValue;
    }

    beforeEach(function(done) {
        localStorage.clear();
        channelId = "myChannel" + Date.now();
        channelUrl = provisioning.bounceProxyUrl + "/channels/" + channelId;
        channelCreationTimeout_ms = 1000;
        channelCreationRetryDelay_ms = 50;
        var channelQos = {
            creationTimeout_ms: channelCreationTimeout_ms,
            creationRetryDelay_ms: channelCreationRetryDelay_ms
        };

        joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello"
        });
        joynrMessage.expiryDate = 9360686108031;
        joynrMessage.replyChannelId = "me";

        // instantiate CommunicationModule spy
        atmosphereSpy = jasmine.createSpyObj("atmosphere", ["subscribe", "unsubscribeUrl"]);
        communicationModuleSpy = jasmine.createSpyObj("communicationModule", ["createXMLHTTPRequest"]);
        communicationModuleSpy.atmosphere = atmosphereSpy;

        longPollingChannelMessageReceiver = new LongPollingChannelMessageReceiver({
            persistency: localStorage,
            bounceProxyUrl: provisioning.bounceProxyUrl,
            communicationModule: communicationModuleSpy,
            channelQos: channelQos
        });
        done();
    });

    it("create fails only if channelCreationTimeout_ms exceed", function(done) {
        var createChannelCallTimestamp;

        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
            Promise.reject(new Error("fake http request failed"))
        );
        createChannelCallTimestamp = Date.now();
        longPollingChannelMessageReceiver
            .create(channelId)
            .then(function() {
                fail("create longPollingChannelMessageReceiver succeeded");
                return null;
            })
            .catch(function() {
                expect(Date.now() - createChannelCallTimestamp + 1).toBeGreaterThan(channelCreationTimeout_ms);
                done();
            });
    });

    it("is instantiable and has all members", function(done) {
        expect(longPollingChannelMessageReceiver).toBeDefined();
        expect(longPollingChannelMessageReceiver instanceof LongPollingChannelMessageReceiver).toBeTruthy();
        expect(longPollingChannelMessageReceiver.create).toBeDefined();
        expect(longPollingChannelMessageReceiver.start).toBeDefined();
        expect(longPollingChannelMessageReceiver.clear).toBeDefined();
        expect(longPollingChannelMessageReceiver.stop).toBeDefined();
        expect(typeof longPollingChannelMessageReceiver.create === "function").toBeTruthy();
        expect(typeof longPollingChannelMessageReceiver.start === "function").toBeTruthy();
        expect(typeof longPollingChannelMessageReceiver.clear === "function").toBeTruthy();
        expect(typeof longPollingChannelMessageReceiver.stop === "function").toBeTruthy();
        done();
    });

    it("create is successful if channel is created", function(done) {
        createChannel()
            .then(function(spy) {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onFulfilled).toHaveBeenCalledWith(channelUrl);
                expect(spy.onRejected).not.toHaveBeenCalled();
                expect(communicationModuleSpy.createXMLHTTPRequest).toHaveBeenCalled();
                expect(communicationModuleSpy.createXMLHTTPRequest).toHaveBeenCalledWith(jasmine.any(Object));
                done();
                return null;
            })
            .catch(function(error) {
                fail("spy not returned: error " + error);
                return null;
            });
    });

    it("create is unsuccessful if channel is not created", function(done) {
        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
            Promise.reject(
                {
                    status: 500,
                    responseText: "responseText"
                },
                "errorThrown"
            )
        );
        longPollingChannelMessageReceiver
            .create(channelId)
            .then(function() {
                fail("create longPollingChannelMessageReceiver unexpectedly successfull");
                return null;
            })
            .catch(function(error) {
                expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                done();
                return null;
            });
    });

    it("after a call to start, messages are transmitted", function(done) {
        var messageSpy = jasmine.createSpy("messageSpy");
        longPollingChannelMessageReceiver.start(messageSpy);
        expect(atmosphereSpy.subscribe).toHaveBeenCalled();
        expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalled();

        var messageCallback = atmosphereSpy.subscribe.calls.argsFor(0)[0].onMessage;
        expect(messageSpy).not.toHaveBeenCalled();
        messageCallback({
            status: 200,
            responseBody: JSON.stringify(joynrMessage)
        });
        expect(messageSpy).toHaveBeenCalled();
        // Doesn't work because, probably because of JoynrMessage magic: expect(messageSpy).toHaveBeenCalledWith(joynrMessage);
        expect(JSON.stringify(messageSpy.calls.argsFor(0)[0])).toEqual(JSON.stringify(joynrMessage));
        done();
    });

    it("clear is successful if channel is cleared", function(done) {
        var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        createChannel()
            .then(function() {
                communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
                    Promise.resolve({
                        getResponseHeader: function() {
                            return channelUrl;
                        }
                    })
                );
                longPollingChannelMessageReceiver
                    .clear()
                    .then(spy.onFulfilled)
                    .catch(spy.onRejected)
                    .catch(outputPromiseError);

                return waitsFor(
                    function() {
                        return spy.onFulfilled.calls.count() > 0;
                    },
                    "channel is cleared",
                    provisioning.ttl
                );
            })
            .then(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onFulfilled).toHaveBeenCalledWith(undefined);
                expect(spy.onRejected).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(function(error) {
                fail("create Channel failed: " + error);
                return null;
            });
    });

    it("clear is unsuccessful if channel is not cleared", function(done) {
        createChannel()
            .then(function() {
                var spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
                communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
                    Promise.reject(
                        {
                            status: 500,
                            responseText: "responseText"
                        },
                        "errorThrown"
                    )
                );
                longPollingChannelMessageReceiver
                    .clear()
                    .then(function() {
                        fail("clear returned successfully");
                        return null;
                    })
                    .catch(function(error) {
                        expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                        done();
                        return null;
                    });
                return null;
            })
            .catch(function(error) {
                fail("create channel failed");
                return null;
            });
    });

    it("stops", function(done) {
        createChannel()
            .then(function() {
                longPollingChannelMessageReceiver.stop();
                expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalled();
                expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalledWith(channelUrl);
                done();
                return null;
            })
            .catch(function(error) {
                fail("create channel failed");
                return null;
            });
    });
});
