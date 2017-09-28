/*jslint node: true*/
/*global console: true */
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
var JoynrMessage = require('../../../classes/joynr/messaging/JoynrMessage');

    describe("libjoynr-js.joynr.messaging.JoynrMessage", function() {

        function getTestMessageFields() {
            var suffix = Date.now();
            return {
                to : "to" + suffix,
                from : "from" + suffix,
                expiryDate : Date.now(),
                replyChannelId : "replyChannelId" + suffix,
                requestReplyId : "requestReplyId" + suffix
            };

        }

        it("is instantiable", function(done) {
            expect(new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            })).toBeDefined();
            done();
        });

        it("is of correct type", function(done) {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            expect(joynrMessage).toBeDefined();
            expect(joynrMessage).not.toBeNull();
            expect(typeof joynrMessage === "object").toBeTruthy();
            expect(joynrMessage instanceof JoynrMessage).toEqual(true);
            done();
        });

        it("constructs with correct member values", function(done) {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            /*jslint newcap: true, nomen: true */
            expect(joynrMessage._typeName).toEqual("joynr.JoynrMessage");
            /*jslint newcap: false, nomen: false */
            expect(joynrMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            expect(joynrMessage.compress).toEqual(false);
            done();
        });

        it("has a header that can be set", function(done) {
            var payload = "hello";
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload : payload
            });
            var fields = getTestMessageFields();

            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE, fields.expiryDate);
            joynrMessage.setHeader(
                    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                    fields.replyChannelId);

            expect(joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE]).toEqual(
                    fields.expiryDate);
            expect(joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID]).toEqual(
                    fields.replyChannelId);

            expect(joynrMessage.payload).toEqual(payload);
            done();
        });

        it("allows setting custom headers", function(done) {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            var headerKey = "headerKey";
            var customHeaderKey = "custom-" + headerKey;
            var customHeaders = {};
            customHeaders[headerKey] = "customHeaderValue";

            joynrMessage.setCustomHeaders(customHeaders);
            expect(joynrMessage.header[customHeaderKey]).toEqual(customHeaders.headerKey);
            done();
        });

        it("allows getting custom headers", function(done) {
            var retrievedCustomHeaders;
            var headerKey = "headerKey";
            var customHeaderKey = "custom-" + headerKey;
            var headerValue = "headerValue";
            var myCustomHeaders = {};
            myCustomHeaders[headerKey] = "customHeaderValue";

            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            joynrMessage.setCustomHeaders(myCustomHeaders);
            joynrMessage.setHeader(headerKey, headerValue);

            retrievedCustomHeaders = joynrMessage.getCustomHeaders();

            expect(retrievedCustomHeaders[headerKey]).toEqual(myCustomHeaders[headerKey]);
            expect(retrievedCustomHeaders[customHeaderKey]).not.toBeDefined();
            expect(joynrMessage.header[customHeaderKey]).toEqual(myCustomHeaders[headerKey]);
            expect(joynrMessage.header[headerKey]).toEqual(headerValue);
            done();
        });

        it("has a payload that can be set", function() {
            var payload = "hello";
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload : payload
            });
            expect(joynrMessage.payload).toEqual(payload);
        });

        it("allows to change receivedFromGlobal", function() {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload : "hello"
            });
            expect(joynrMessage.isReceivedFromGlobal).toBe(false);
            joynrMessage.setReceivedFromGlobal(true);
            expect(joynrMessage.isReceivedFromGlobal).toBe(true);
        });

        it("allows to change isLocalMessage", function() {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload : "hello"
            });
            expect(joynrMessage.isLocalMessage).toBe(false);
            joynrMessage.setIsLocalMessage(true);
            expect(joynrMessage.isLocalMessage).toBe(true);
        });

        it("allows to change compress", function() {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
                payload : "hello"
            });
            expect(joynrMessage.compress).toBe(false);
            joynrMessage.setCompress(true);
            expect(joynrMessage.compress).toBe(true);
        });

        it("has comfort functions for setting values", function() {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            var fields = getTestMessageFields();

            joynrMessage.to = fields.to;
            joynrMessage.from = fields.from;
            joynrMessage.expiryDate = fields.expiryDate;
            joynrMessage.replyChannelId = fields.replyChannelId;
        });

        it("has members that can be stringified to json", function() {
            var joynrMessage = new JoynrMessage({
                type : JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            });
            var fields = getTestMessageFields();

            joynrMessage.to = fields.to;
            joynrMessage.from = fields.from;

            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE, fields.expiryDate);
            joynrMessage.setHeader(
                    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                    fields.replyChannelId);

            // stringify and parse to create a new copy
            var json = JSON.stringify(joynrMessage);
            var newJoynrMessage = JSON.parse(json);

            expect(newJoynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE]).toEqual(
                    fields.expiryDate);

            expect(newJoynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID])
                    .toEqual(fields.replyChannelId);

            expect(newJoynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_FROM_PARTICIPANT_ID])
                    .toEqual(fields.from);

            expect(newJoynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID])
                    .toEqual(fields.to);
        });

    });
