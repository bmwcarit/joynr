/* eslint-disable @typescript-eslint/no-non-null-assertion */
/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

/*eslint no-use-before-define: "off"*/

import LoggingManager from "../../system/LoggingManager";
const log = LoggingManager.getLogger("joynr.messaging.socket.MagicCookieUtil");

class MagicCookieUtil {
    // range: [BEGIN, END[
    public static OFFSET_BEGIN_COOKIE = 0;
    public static OFFSET_END_COOKIE = 4;
    public static OFFSET_BEGIN_PAYLOAD_LENGTH = 4;
    public static OFFSET_BEGIN_PAYLOAD = 8;

    /** Magic cookie precedes every init-frame */
    public static INIT_COOKIE = `MJI1`;
    /** Magic cookie precedes every message-frame */
    public static MESSAGE_COOKIE = `MJM1`;

    /** placeholder for magic cookie is 4 bytes + placeholder for payload length is 4 bytes */
    public static MAGIC_COOKIE_AND_PAYLOAD_LENGTH = 8;

    public static getPayloadBuff(data: Buffer): Buffer {
        return Buffer.from(data.subarray(this.OFFSET_BEGIN_PAYLOAD, data.length));
    }

    public static getPayloadLength(data: Buffer): number {
        const infoLengthAsString: any = data.readUInt32BE(MagicCookieUtil.OFFSET_BEGIN_PAYLOAD_LENGTH);
        return parseInt(infoLengthAsString);
    }

    public static getMagicCookieBuff(data: Buffer): Buffer {
        return Buffer.from(data.subarray(this.OFFSET_BEGIN_COOKIE, this.OFFSET_END_COOKIE));
    }

    public static writeMagicCookies(serializedJoynrMessageBuff: Buffer, magicCookie: string): Buffer {
        const payloadLengthBuff = Buffer.alloc(4, 0); // 4 bytes, initialized with 0
        payloadLengthBuff.writeUInt32BE(serializedJoynrMessageBuff.length, 0);

        const magicCookieBuff = Buffer.from(magicCookie);
        const arrayOfBuffers = [magicCookieBuff, payloadLengthBuff, serializedJoynrMessageBuff];
        return Buffer.concat(arrayOfBuffers);
    }

    public static checkCookie(internBuff: Buffer): boolean {
        const magicCookie: string = this.getMagicCookieBuff(internBuff).toString();
        log.debug(`received this MagicCookie: ${magicCookie}`);
        if (magicCookie !== this.MESSAGE_COOKIE) {
            const errorMsg: string = `Invalid cookies. Cookie type must be ${
                this.MESSAGE_COOKIE
            }, received : ${magicCookie}, connection closed`;
            log.fatal(errorMsg);
            return false;
        }
        return true;
    }
}

export = MagicCookieUtil;
