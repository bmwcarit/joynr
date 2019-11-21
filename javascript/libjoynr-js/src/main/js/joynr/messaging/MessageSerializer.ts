/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
/*eslint global-require: "off"*/
import JoynrMessage from "./JoynrMessage";

import LoggingManager from "../system/LoggingManager";
const log = LoggingManager.getLogger("joynr.messaging.MessageSerializer");

import * as smrf from "smrf";

function serializeSmrfMessage(smrfMsg: JoynrMessage): Buffer {
    try {
        return smrf.serialize(smrfMsg);
    } catch (e) {
        throw new Error(`smrf.serialize: got exception ${e}`);
    }
}

function deserializeSmrfMessage(data: Buffer): JoynrMessage {
    try {
        return smrf.deserialize(data);
    } catch (e) {
        throw new Error(`smrf.deserialize: got exception ${e}`);
    }
}

export function stringify(joynrMessage: JoynrMessage): Buffer {
    // @ts-ignore
    joynrMessage.body = joynrMessage.body || new Buffer(joynrMessage.payload);

    return serializeSmrfMessage(joynrMessage);
}

export function parse(data: Buffer): JoynrMessage | undefined {
    if (typeof data !== "object") {
        log.error("MessageSerializer received unsupported message.");
    } else {
        const smrfMsg = deserializeSmrfMessage(data);
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        const expiryDate = smrfMsg.isTtlAbsolute === true ? smrfMsg.ttlMs : smrfMsg.ttlMs! + Date.now();

        // smrfMsg has two attributes we need to throw away -> create new Object
        const messageWithoutRest = {
            headers: smrfMsg.headers,
            sender: smrfMsg.sender,
            recipient: smrfMsg.recipient,
            ttlMs: expiryDate,
            // @ts-ignore
            payload: smrfMsg.body.toString()
        };
        return JoynrMessage.parseMessage(messageWithoutRest);
    }
}
