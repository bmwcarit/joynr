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

joynrTestRequire("joynr/messaging/TestJoynrMessage", [ "joynr/messaging/JoynrMessage"
], function(JoynrMessage) {

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

        it("is instantiable", function() {
            expect(new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST)).toBeDefined();
        });

        it("is of correct type", function() {
            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            expect(joynrMessage).toBeDefined();
            expect(joynrMessage).not.toBeNull();
            expect(typeof joynrMessage === "object").toBeTruthy();
            expect(joynrMessage instanceof JoynrMessage).toEqual(true);
        });

        it("constructs with correct member values", function() {
            var messageType = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;
            var joynrMessage = new JoynrMessage(messageType);
            /*jslint newcap: true, nomen: true */
            expect(joynrMessage._typeName).toEqual("joynr.JoynrMessage");
            /*jslint newcap: false, nomen: false */
            expect(joynrMessage.type).toEqual(messageType);
        });

        it("has members that cannot be changed after initialization", function() {
            var messageType = JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST;
            var joynrMessage = new JoynrMessage(messageType);
            var messageHeader;

            // the following setting os fields should have no effect. These are not writable.
            /*jslint newcap: true, nomen: true */
            joynrMessage._typeName = "nonsense";
            expect(joynrMessage._typeName).toEqual("joynr.JoynrMessage");
            /*jslint newcap: false, nomen: false */

            joynrMessage.type = "nonsense";
            expect(joynrMessage.type).toEqual(messageType);

            messageHeader = joynrMessage.header;
            joynrMessage.header = "nonsense";
            expect(joynrMessage.header).toEqual(messageHeader);

        });

        it("has a header that can be set", function() {
            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            var fields = getTestMessageFields();

            joynrMessage.setHeader(JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE, fields.expiryDate);
            joynrMessage.setHeader(
                    JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID,
                    fields.replyChannelId);

            expect(joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_EXPIRYDATE]).toEqual(
                    fields.expiryDate);
            expect(joynrMessage.header[JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID]).toEqual(
                    fields.replyChannelId);

            var payload = "hello";
            joynrMessage.payload = payload;
            expect(joynrMessage.payload).toEqual(payload);
        });

        it("allows setting custom headers", function() {
            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            var headerKey = "headerKey";
            var customHeaderKey = "custom-" + headerKey;
            var customHeaders = {};
            customHeaders[headerKey] = "customHeaderValue";

            joynrMessage.setCustomHeaders(customHeaders);
            expect(joynrMessage.header[customHeaderKey]).toEqual(customHeaders.headerKey);
        });

        it("allows getting custom headers", function() {
            var retrievedCustomHeaders;
            var headerKey = "headerKey";
            var customHeaderKey = "custom-" + headerKey;
            var headerValue = "headerValue";
            var myCustomHeaders = {};
            myCustomHeaders[headerKey] = "customHeaderValue";

            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            joynrMessage.setCustomHeaders(myCustomHeaders);
            joynrMessage.setHeader(headerKey, headerValue);

            retrievedCustomHeaders = joynrMessage.getCustomHeaders();

            expect(retrievedCustomHeaders[headerKey]).toEqual(myCustomHeaders[headerKey]);
            expect(retrievedCustomHeaders[customHeaderKey]).not.toBeDefined();
            expect(joynrMessage.header[customHeaderKey]).toEqual(myCustomHeaders[headerKey]);
            expect(joynrMessage.header[headerKey]).toEqual(headerValue);
        });

        it("has a payload that can be set", function() {
            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            var payload = "hello";
            joynrMessage.payload = payload;
            expect(joynrMessage.payload).toEqual(payload);
        });

        it("has comfort functions for setting values", function() {
            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
            var fields = getTestMessageFields();

            joynrMessage.to = fields.to;
            joynrMessage.from = fields.from;
            joynrMessage.expiryDate = fields.expiryDate;
            joynrMessage.replyChannelId = fields.replyChannelId;
        });

        it("can be of type request", function() {});

        it("has members that can be stringified to json", function() {
            var joynrMessage = new JoynrMessage(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
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
});