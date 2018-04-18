/* eslint prefer-promise-reject-errors: "off" */
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
const LongPollingChannelMessageReceiver = require("../../../../../main/js/joynr/messaging/channel/LongPollingChannelMessageReceiver");
const JoynrMessage = require("../../../../../main/js/joynr/messaging/JoynrMessage");
const LocalStorage = require("../../../../../test/js/global/LocalStorageNodeTests");
const provisioning = require("../../../../resources/joynr/provisioning/provisioning_root");
const waitsFor = require("../../../../../test/js/global/WaitsFor");
const localStorage = new LocalStorage();

describe("libjoynr-js.joynr.messaging.LongPollingChannelMessageReceiver", () => {
    let atmosphereSpy, communicationModuleSpy, longPollingChannelMessageReceiver;
    let channelId, channelUrl, joynrMessage;
    let channelCreationTimeout_ms, channelCreationRetryDelay_ms;

    function outputPromiseError(error) {
        expect(error.toString()).toBeFalsy();
    }

    function createChannel() {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);

        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
            Promise.resolve({
                getResponseHeader() {
                    return channelUrl;
                }
            })
        );
        longPollingChannelMessageReceiver
            .create(channelId)
            .then(spy.onFulfilled)
            .catch(spy.onRejected)
            .catch(outputPromiseError);

        const returnValue = waitsFor(
            () => {
                return spy.onFulfilled.calls.count() > 0;
            },
            "channel is created",
            provisioning.ttl
        ).then(() => Promise.resolve(spy));
        return returnValue;
    }

    beforeEach(done => {
        localStorage.clear();
        channelId = "myChannel" + Date.now();
        channelUrl = provisioning.bounceProxyUrl + "/channels/" + channelId;
        channelCreationTimeout_ms = 1000;
        channelCreationRetryDelay_ms = 50;
        const channelQos = {
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
            channelQos
        });
        done();
    });

    it("create fails only if channelCreationTimeout_ms exceed", done => {
        let createChannelCallTimestamp;

        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
            Promise.reject(new Error("fake http request failed"))
        );
        createChannelCallTimestamp = Date.now();
        longPollingChannelMessageReceiver
            .create(channelId)
            .then(() => {
                fail("create longPollingChannelMessageReceiver succeeded");
                return null;
            })
            .catch(() => {
                expect(Date.now() - createChannelCallTimestamp + 1).toBeGreaterThan(channelCreationTimeout_ms);
                done();
            });
    });

    it("is instantiable and has all members", done => {
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

    it("create is successful if channel is created", done => {
        createChannel()
            .then(spy => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onFulfilled).toHaveBeenCalledWith(channelUrl);
                expect(spy.onRejected).not.toHaveBeenCalled();
                expect(communicationModuleSpy.createXMLHTTPRequest).toHaveBeenCalled();
                expect(communicationModuleSpy.createXMLHTTPRequest).toHaveBeenCalledWith(jasmine.any(Object));
                done();
                return null;
            })
            .catch(error => {
                fail("spy not returned: error " + error);
                return null;
            });
    });

    it("create is unsuccessful if channel is not created", done => {
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
            .then(() => {
                fail("create longPollingChannelMessageReceiver unexpectedly successfull");
                return null;
            })
            .catch(error => {
                expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                done();
                return null;
            });
    });

    it("after a call to start, messages are transmitted", done => {
        const messageSpy = jasmine.createSpy("messageSpy");
        longPollingChannelMessageReceiver.start(messageSpy);
        expect(atmosphereSpy.subscribe).toHaveBeenCalled();
        expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalled();

        const messageCallback = atmosphereSpy.subscribe.calls.argsFor(0)[0].onMessage;
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

    it("clear is successful if channel is cleared", done => {
        const spy = jasmine.createSpyObj("spy", ["onFulfilled", "onRejected"]);
        createChannel()
            .then(() => {
                communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
                    Promise.resolve({
                        getResponseHeader() {
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
                    () => {
                        return spy.onFulfilled.calls.count() > 0;
                    },
                    "channel is cleared",
                    provisioning.ttl
                );
            })
            .then(() => {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onFulfilled).toHaveBeenCalledWith(undefined);
                expect(spy.onRejected).not.toHaveBeenCalled();
                done();
                return null;
            })
            .catch(error => {
                fail("create Channel failed: " + error);
                return null;
            });
    });

    it("clear is unsuccessful if channel is not cleared", done => {
        createChannel()
            .then(() => {
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
                    .then(() => {
                        fail("clear returned successfully");
                        return null;
                    })
                    .catch(error => {
                        expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                        done();
                        return null;
                    });
                return null;
            })
            .catch(error => {
                fail("create channel failed: " + error);
            });
    });

    it("stops", done => {
        createChannel()
            .then(() => {
                longPollingChannelMessageReceiver.stop();
                expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalled();
                expect(atmosphereSpy.unsubscribeUrl).toHaveBeenCalledWith(channelUrl);
                done();
                return null;
            })
            .catch(error => {
                fail("create channel failed: " + error);
            });
    });
});
