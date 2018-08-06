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
require("../../node-unit-test-helper");
const JoynrMessage = require("../../../../main/js/joynr/messaging/JoynrMessage");

describe("libjoynr-js.joynr.messaging.JoynrMessage", () => {
    function getTestMessageFields() {
        const suffix = Date.now();
        return {
            to: `to${suffix}`,
            from: `from${suffix}`,
            expiryDate: Date.now(),
            replyChannelId: `replyChannelId${suffix}`,
            requestReplyId: `requestReplyId${suffix}`
        };
    }

    it("is instantiable", done => {
        expect(
            new JoynrMessage({
                type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
            })
        ).toBeDefined();
        done();
    });

    it("is of correct type", done => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        expect(joynrMessage).toBeDefined();
        expect(joynrMessage).not.toBeNull();
        expect(typeof joynrMessage === "object").toBeTruthy();
        expect(joynrMessage instanceof JoynrMessage).toEqual(true);
        done();
    });

    it("constructs with correct member values", done => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        expect(joynrMessage._typeName).toEqual("joynr.JoynrMessage");
        expect(joynrMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST);
        expect(joynrMessage.compress).toEqual(false);
        done();
    });

    it("has a header that can be set", done => {
        const payload = "hello";
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload
        });
        const fields = getTestMessageFields();

        joynrMessage.expiryDate = fields.expiryDate;
        joynrMessage.replyChannelId = fields.replyChannelId;

        expect(joynrMessage.expiryDate).toEqual(fields.expiryDate);
        expect(joynrMessage.replyChannelId).toEqual(fields.replyChannelId);

        expect(joynrMessage.payload).toEqual(payload);
        done();
    });

    it("allows setting custom headers", done => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        const headerKey = "headerKey";
        const customHeaderKey = `c-${headerKey}`;
        const customHeaders = {};
        customHeaders[headerKey] = "customHeaderValue";

        joynrMessage.setCustomHeaders(customHeaders);
        expect(joynrMessage.getHeader(customHeaderKey)).toEqual(customHeaders.headerKey);
        done();
    });

    it("allows getting custom headers", done => {
        const headerKey = "headerKey";
        const customHeaderKey = `c-${headerKey}`;
        const headerValue = "headerValue";
        const myCustomHeaders = {};
        myCustomHeaders[headerKey] = "customHeaderValue";

        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        joynrMessage.setCustomHeaders(myCustomHeaders);
        joynrMessage.setHeader(headerKey, headerValue);

        const retrievedCustomHeaders = joynrMessage.getCustomHeaders();

        expect(retrievedCustomHeaders[headerKey]).toEqual(myCustomHeaders[headerKey]);
        expect(retrievedCustomHeaders[customHeaderKey]).not.toBeDefined();
        expect(joynrMessage.getHeader(customHeaderKey)).toEqual(myCustomHeaders[headerKey]);
        expect(joynrMessage.getHeader(headerKey)).toEqual(headerValue);
        done();
    });

    it("has a payload that can be set", () => {
        const payload = "hello";
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload
        });
        expect(joynrMessage.payload).toEqual(payload);
    });

    it("allows to change receivedFromGlobal", () => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello"
        });
        expect(joynrMessage.isReceivedFromGlobal).toBe(false);
        joynrMessage.isReceivedFromGlobal = true;
        expect(joynrMessage.isReceivedFromGlobal).toBe(true);
    });

    it("allows to change isLocalMessage", () => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello"
        });
        expect(joynrMessage.isLocalMessage).toBe(false);
        joynrMessage.isLocalMessage = true;
        expect(joynrMessage.isLocalMessage).toBe(true);
    });

    it("allows to change compress", () => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: "hello"
        });
        expect(joynrMessage.compress).toBe(false);
        joynrMessage.compress = true;
        expect(joynrMessage.compress).toBe(true);
    });

    it("has comfort functions for setting values", () => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        const fields = getTestMessageFields();

        joynrMessage.to = fields.to;
        joynrMessage.from = fields.from;
        joynrMessage.expiryDate = fields.expiryDate;
        joynrMessage.replyChannelId = fields.replyChannelId;
    });

    it("has members that can be stringified to json", () => {
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
        });
        const fields = getTestMessageFields();

        joynrMessage.to = fields.to;
        joynrMessage.from = fields.from;

        joynrMessage.expiryDate = fields.expiryDate;
        joynrMessage.replyChannelId = fields.replyChannelId;

        // stringify and parse to create a new copy
        const json = JSON.stringify(joynrMessage);
        const newJoynrMessage = JoynrMessage.parseMessage(JSON.parse(json));

        expect(newJoynrMessage.expiryDate).toEqual(fields.expiryDate);

        expect(newJoynrMessage.replyChannelId).toEqual(fields.replyChannelId);

        expect(newJoynrMessage.from).toEqual(fields.from);

        expect(newJoynrMessage.to).toEqual(fields.to);
    });
});
