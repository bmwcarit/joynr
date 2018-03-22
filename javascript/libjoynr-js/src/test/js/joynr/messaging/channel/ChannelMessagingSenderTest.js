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
require("../../../node-unit-test-helper");
var Promise = require("../../../../classes/global/Promise");
var ChannelMessagingSender = require("../../../../classes/joynr/messaging/channel/ChannelMessagingSender");
var JoynrMessage = require("../../../../classes/joynr/messaging/JoynrMessage");
var ChannelAddress = require("../../../../classes/joynr/system/RoutingTypes/ChannelAddress");
var Typing = require("../../../../classes/joynr/util/Typing");
var LoggerFactory = require("../../../../classes/joynr/system/LoggerFactory");
var provisioningRoot = require("../../../../test-classes/joynr/provisioning/provisioning_root");
var waitsFor = require("../../../../test-classes/global/WaitsFor");

var log = LoggerFactory.getLogger("joynr.messaging.TestChannelMessagingSender");

describe("libjoynr-js.joynr.messaging.ChannelMessagingSender", function() {
    var communicationModuleSpy, channelMessageSender;
    var channelAddress, channelUrlInformation, joynrMessage;
    var resendDelay_ms;

    function outputPromiseError(error) {
        expect(error.toString()).toBeFalsy();
    }

    beforeEach(function(done) {
        resendDelay_ms = 500;
        channelAddress = new ChannelAddress({
            messagingEndpointUrl: "http://testurl.com",
            channelId: "myChannel" + Date.now()
        });
        var channelQos = {
            resendDelay_ms: resendDelay_ms
        };

        joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello"
        });
        joynrMessage.expiryDate = 9360686108031;
        joynrMessage.replyChannelId = "me";

        // instantiate spies
        communicationModuleSpy = jasmine.createSpyObj("communicationModule", ["createXMLHTTPRequest"]);

        channelMessageSender = new ChannelMessagingSender({
            communicationModule: communicationModuleSpy,
            channelQos: channelQos
        });
        done();
    });

    it("is instantiable and has all members", function(done) {
        expect(ChannelMessagingSender).toBeDefined();
        expect(typeof ChannelMessagingSender === "function").toBeTruthy();
        expect(channelMessageSender).toBeDefined();
        expect(channelMessageSender instanceof ChannelMessagingSender).toBeTruthy();
        expect(channelMessageSender.send).toBeDefined();
        expect(typeof channelMessageSender.send === "function").toBeTruthy();
        done();
    });

    it(
        "if communicationModule.createXMLHTTPRequest call fails, channelMessageSender only fails if message expires",
        function(done) {
            var timeStamp;
            var relativeExpiryDate = resendDelay_ms * 3;

            communicationModuleSpy.createXMLHTTPRequest.and.returnValue(
                Promise.reject(
                    {
                        status: 500,
                        responseText: "responseText",
                        statusText: "errorThrown"
                    },
                    "errorStatus"
                )
            );
            joynrMessage.expiryDate = Date.now() + relativeExpiryDate;
            channelMessageSender.start();
            timeStamp = Date.now();
            channelMessageSender
                .send(joynrMessage, channelAddress)
                .then(fail)
                .catch(function(error) {
                    expect(joynrMessage.expiryDate).toBeLessThan(Date.now() + 1);
                    expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                    expect(communicationModuleSpy.createXMLHTTPRequest).toHaveBeenCalled();
                    done();
                    return null;
                });
        },
        resendDelay_ms * 3 * 5
    );

    it("if channelMessageSender.send fails after expiryDate, if ChannelMessagingSender is not started", function(done) {
        joynrMessage.expiryDate = Date.now() + resendDelay_ms;
        channelMessageSender
            .send(joynrMessage, channelAddress)
            .then(fail)
            .catch(function(error) {
                expect(joynrMessage.expiryDate).toBeLessThan(Date.now() + 1);
                expect(Object.prototype.toString.call(error) === "[object Error]").toBeTruthy();
                expect(communicationModuleSpy.createXMLHTTPRequest).not.toHaveBeenCalled();
                done();
                return null;
            });
    });

    it("sends message using communicationModule.createXMLHTTPRequest", function(done) {
        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(Promise.resolve());
        channelMessageSender.start();
        channelMessageSender
            .send(joynrMessage, channelAddress)
            .then(function(result) {
                expect(result).toEqual(undefined);
                expect(communicationModuleSpy.createXMLHTTPRequest).toHaveBeenCalled();
                done();
                return null;
            })
            .catch(fail);
    });

    it("reject queued messages once shut down", function(done) {
        communicationModuleSpy.createXMLHTTPRequest.and.returnValue(Promise.resolve());
        var sendPromise = channelMessageSender.send(joynrMessage, channelAddress);
        expect(communicationModuleSpy.createXMLHTTPRequest).not.toHaveBeenCalled();
        channelMessageSender.shutdown();
        sendPromise.then(fail).catch(done);
    });
});
