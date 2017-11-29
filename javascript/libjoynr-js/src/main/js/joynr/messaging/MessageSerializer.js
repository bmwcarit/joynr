/*jslint es5: true, nomen: true, node: true */
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
var JSONSerializer = require("../util/JSONSerializer");
var JoynrMessage = require("./JoynrMessage");
var LoggerFactory = require("../system/LoggerFactory");
var log = LoggerFactory.getLogger("joynr.messaging.MessageSerializer");

var MessageSerializer = {};

function useSmrf() {
    var smrf = require("../../global/SmrfNode");

    var skipJoynrHeaderKeys = {
        contentType: true,
        creator: true,
        effort: true,
        from: true,
        msgId: true,
        replyChannelId: true,
        to: true,
        expiryDate: true
    };

    var skipSmrfHeaderKeys = {
        // headers already converted manually
        t: true,
        id: true,
        re: true,
        ef: true,
        // reserved headers, prevent overwriting
        from: true,
        to: true,
        msgId: true,
        replyChannelId: true,
        expiryDate: true,
        effort: true
    };

    MessageSerializer.stringify = function(joynrMessage, signingCallback) {
        var smrfMsg = {};
        var headerKey;
        smrfMsg.sender = joynrMessage.header.from;
        smrfMsg.recipient = joynrMessage.header.to;
        smrfMsg.ttlMs = joynrMessage.header.expiryDate;
        smrfMsg.isTtlAbsolute = true;
        smrfMsg.isCompressed = joynrMessage.compress;
        smrfMsg.body = new Buffer(joynrMessage.payload);
        smrfMsg.encryptionCert = null;
        smrfMsg.signingCert = null;
        smrfMsg.signingKey = null;
        smrfMsg.headers = {};
        smrfMsg.headers.t = joynrMessage.type;
        smrfMsg.headers.id = joynrMessage.header.msgId;
        if (signingCallback) {
            smrfMsg.signingCallback = signingCallback;
        }
        if (joynrMessage.header.replyChannelId) {
            smrfMsg.headers.re = joynrMessage.header.replyChannelId;
        }
        if (joynrMessage.header.effort) {
            smrfMsg.headers.ef = joynrMessage.header.effort;
        }
        for (headerKey in joynrMessage.header) {
            if (joynrMessage.header.hasOwnProperty(headerKey)) {
                if (!skipJoynrHeaderKeys[headerKey]) {
                    smrfMsg.headers[headerKey] = joynrMessage.header[headerKey];
                }
            }
        }
        var serializedMsg;
        try {
            serializedMsg = smrf.serialize(smrfMsg);
        } catch (e) {
            throw new Error("ws.marshalJoynrMessage: got exception " + e);
        }
        return serializedMsg;
    };

    MessageSerializer.parse = function(data) {
        if (typeof data !== "object") {
            log.error("MessageSerializer received unsupported message.");
        } else {
            var headerKey;
            var smrfMsg;
            try {
                smrfMsg = smrf.deserialize(data);
            } catch (e) {
                throw new Error("ws.marshalJoynrMessage: got exception " + e);
            }
            var convertedMsg = {};
            convertedMsg.header = {};
            convertedMsg.header.from = smrfMsg.sender;
            convertedMsg.header.to = smrfMsg.recipient;
            convertedMsg.header.msgId = smrfMsg.headers.id;
            if (smrfMsg.headers.re) {
                convertedMsg.header.replyChannelId = smrfMsg.headers.re;
            }
            convertedMsg.type = smrfMsg.headers.t;
            // ignore for now:
            //   smrfMsg.headers.isCompressed
            //   smrfMsg.headers.encryptionCert
            //   smrfMsg.headers.signingKey
            //   smrfMsg.headers.signingCert
            if (smrfMsg.isTtlAbsolute === true) {
                convertedMsg.header.expiryDate = smrfMsg.ttlMs;
            } else {
                convertedMsg.header.expiryDate = smrfMsg.ttlMs + Date.now();
            }
            convertedMsg.header.effort = smrfMsg.headers.ef;
            convertedMsg.payload = smrfMsg.body.toString();
            for (headerKey in smrfMsg.headers) {
                if (smrfMsg.headers.hasOwnProperty(headerKey)) {
                    if (!skipSmrfHeaderKeys[headerKey]) {
                        convertedMsg.header[headerKey] = smrfMsg.headers[headerKey];
                    }
                }
            }
            return new JoynrMessage(convertedMsg);
        }
    };
}

if (global.window !== undefined) {
    MessageSerializer.stringify = JSONSerializer.stringify;
    MessageSerializer.parse = function(payload) {
        return new JoynrMessage(JSON.parse(payload.toString()));
    };
} else {
    useSmrf();
}

module.exports = MessageSerializer;
