/*eslint global-require: "off"*/
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
const JoynrMessage = require("./JoynrMessage");
const LoggingManager = require("../system/LoggingManager");
const log = LoggingManager.getLogger("joynr.messaging.MessageSerializer");

const MessageSerializer = {};

const smrf = require("smrf");

function serializeSmrfMessage(smrfMsg) {
    try {
        return smrf.serialize(smrfMsg);
    } catch (e) {
        throw new Error(`smrf.serialize: got exception ${e}`);
    }
}

function deserializeSmrfMessage(data) {
    try {
        return smrf.deserialize(data);
    } catch (e) {
        throw new Error(`smrf.deserialize: got exception ${e}`);
    }
}

MessageSerializer.stringify = function(joynrMessage) {
    joynrMessage.body = joynrMessage.body || new Buffer(joynrMessage.payload);

    return serializeSmrfMessage(joynrMessage);
};

MessageSerializer.parse = function(data) {
    if (typeof data !== "object") {
        log.error("MessageSerializer received unsupported message.");
    } else {
        const smrfMsg = deserializeSmrfMessage(data);
        const expiryDate = smrfMsg.isTtlAbsolute === true ? smrfMsg.ttlMs : smrfMsg.ttlMs + Date.now();

        // smrfMsg has two attributes we need to throw away -> create new Object
        const messageWithoutRest = {
            headers: smrfMsg.headers,
            sender: smrfMsg.sender,
            recipient: smrfMsg.recipient,
            ttlMs: expiryDate,
            payload: smrfMsg.body.toString()
        };
        return JoynrMessage.parseMessage(messageWithoutRest);
    }
};

module.exports = MessageSerializer;
