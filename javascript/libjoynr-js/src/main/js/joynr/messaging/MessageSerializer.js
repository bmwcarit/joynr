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

    function serializeSmrfMessage(smrfMsg) {
        try {
            return smrf.serialize(smrfMsg);
        } catch (e) {
            throw new Error("ws.marshalJoynrMessage: got exception " + e);
        }
    }

    function deserializeSmrfMessage(data) {
        try {
            return smrf.deserialize(data);
        } catch (e) {
            throw new Error("ws.marshalJoynrMessage: got exception " + e);
        }
    }

    MessageSerializer.stringify = function(joynrMessage, signingCallback) {
        var smrfMsg = {
            sender: joynrMessage.header.from,
            recipient: joynrMessage.header.to,
            ttlMs: joynrMessage.header.expiryDate,
            isTtlAbsolute: true,
            isCompressed: joynrMessage.compress,
            body: new Buffer(joynrMessage.payload),
            encryptionCert: null,
            signingCert: null,
            signingKey: null,
            headers: {
                t: joynrMessage.type,
                id: joynrMessage.header.msgId
            }
        };

        if (signingCallback) {
            smrfMsg.signingCallback = signingCallback;
        }
        if (joynrMessage.header.replyChannelId) {
            smrfMsg.headers.re = joynrMessage.header.replyChannelId;
        }
        if (joynrMessage.header.effort) {
            smrfMsg.headers.ef = joynrMessage.header.effort;
        }
        var headerKey;
        for (headerKey in joynrMessage.header) {
            if (joynrMessage.header.hasOwnProperty(headerKey)) {
                if (!skipJoynrHeaderKeys[headerKey]) {
                    smrfMsg.headers[headerKey] = joynrMessage.header[headerKey];
                }
            }
        }

        return serializeSmrfMessage(smrfMsg);
    };

    MessageSerializer.parse = function(data) {
        if (typeof data !== "object") {
            log.error("MessageSerializer received unsupported message.");
        } else {
            var headerKey;

            var smrfMsg = deserializeSmrfMessage(data);
            var expiryDate = smrfMsg.isTtlAbsolute === true ? smrfMsg.ttlMs : smrfMsg.ttlMs + Date.now();

            var convertedMsg = {
                header: {
                    from: smrfMsg.sender,
                    to: smrfMsg.recipient,
                    msgId: smrfMsg.headers.id,
                    effort: smrfMsg.headers.ef,
                    expiryDate: expiryDate
                },
                type: smrfMsg.headers.t,
                payload: smrfMsg.body.toString()
            };

            if (smrfMsg.headers.re) {
                convertedMsg.header.replyChannelId = smrfMsg.headers.re;
            }
            // ignore for now:
            //   smrfMsg.headers.isCompressed
            //   smrfMsg.headers.encryptionCert
            //   smrfMsg.headers.signingKey
            //   smrfMsg.headers.signingCert

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
