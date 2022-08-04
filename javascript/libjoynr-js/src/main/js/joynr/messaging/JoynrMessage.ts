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
const MESSAGE_CUSTOM_HEADER_PREFIX = "c-";
import { nanoid } from "nanoid";

const jmBase = nanoid();
let jmIndex = 0;

interface SmrfMessage {
    ttlMs: number;
    recipient: string;
    sender: string;
    isCompressed: boolean;
    creator: string;
    headers: Record<string, any>;
}

class JoynrMessage implements Partial<SmrfMessage> {
    // smrf attributes
    public ttlMs?: number;
    public recipient?: string;
    public sender?: string;
    public isCompressed?: boolean;
    public creator?: string;

    // values with defaults on prototype
    public isLocalMessage?: boolean;
    public isReceivedFromGlobal?: boolean;
    public isTtlAbsolute?: boolean;
    public encryptionCert: any;
    public signingCert: any;
    public signingKey: any;

    public static readonly JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID: string = "to";
    public static readonly JOYNRMESSAGE_HEADER_CONTENT_TYPE: string = "contentType";
    public static readonly JOYNRMESSAGE_HEADER_REPLY_CHANNELID: string = "replyChannelId";
    public static readonly JOYNRMESSAGE_HEADER_EFFORT: string = "effort";
    public static readonly JOYNRMESSAGE_HEADER_CREATOR_USER_ID: string = "creator";
    public static readonly JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP: string = "sst";
    public static readonly JOYNRMESSAGE_TYPE_MULTICAST: string = "m";
    public static readonly JOYNRMESSAGE_TYPE_PUBLICATION: string = "p";
    public static readonly JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY: string = "srp";
    public static readonly JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST: string = "brq";
    public static readonly JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST: string = "mrq";
    public static readonly JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST: string = "arq";
    public static readonly JOYNRMESSAGE_TYPE_REPLY: string = "rp";
    public static readonly JOYNRMESSAGE_TYPE_REQUEST: string = "rq";
    public static readonly JOYNRMESSAGE_TYPE_ONE_WAY: string = "o";

    public headers: Record<string, any> = {};
    public _typeName?: string;
    public payload: any;

    public signingCallback?: Function;

    /**
     * @param payload
     * @param type the message type as defined by JoynrMessage.JOYNRMESSAGE_TYPE_*
     * @param msgId
     */
    public constructor({ payload, type, msgId }: { payload: string; type: string; msgId?: string }) {
        this.payload = payload;
        this.type = type;
        this.msgId = msgId || `${jmBase}_${jmIndex++}`;
    }

    public static setSigningCallback(callback: Function): void {
        JoynrMessage.prototype.signingCallback = callback;
    }

    public static parseMessage(settings: any): JoynrMessage {
        settings.headers = settings.headers || {};
        settings.headers.id = settings.headers.id || `${jmBase}_${jmIndex++}`;
        Object.setPrototypeOf(settings, JoynrMessage.prototype);
        return settings;
    }

    /**
     * @param customHeaders a map containing key/value pairs of headers to be set as custom
     *            headers. The keys will be added to the header field with the prefix
     *            MESSAGE_CUSTOM_HEADER_PREFIX
     * @returns joynrMessage instance for chainable calls
     */
    public setCustomHeaders(customHeaders: Record<string, any>): JoynrMessage {
        let headerKey;
        for (headerKey in customHeaders) {
            if (Object.prototype.hasOwnProperty.call(customHeaders, headerKey)) {
                this.headers[MESSAGE_CUSTOM_HEADER_PREFIX + headerKey] = customHeaders[headerKey];
            }
        }
        return this;
    }

    /**
     * @returns customHeader object containing all headers that begin with the
     *          prefix MESSAGE_CUSTOM_HEADER_PREFIX
     */
    public getCustomHeaders(): Record<string, any> {
        const customHeaders: Record<string, any> = {};
        for (const headerKey in this.headers) {
            if (
                Object.prototype.hasOwnProperty.call(this.headers, headerKey) &&
                headerKey.substr(0, MESSAGE_CUSTOM_HEADER_PREFIX.length) === MESSAGE_CUSTOM_HEADER_PREFIX
            ) {
                const trimmedKey = headerKey.substr(MESSAGE_CUSTOM_HEADER_PREFIX.length);

                customHeaders[trimmedKey] = this.headers[headerKey];
            }
        }
        return customHeaders;
    }

    /**
     * @param key is one of the header keys defined in JoynrMessagingDefines
     * @param value of the header
     * @returns joynrMessage instance for chainable calls
     */
    public setHeader(key: string, value: any): JoynrMessage {
        this.headers[key] = value;
        return this;
    }

    /**
     * @param key is one of the header keys defined in JoynrMessagingDefines
     * @returns value of header key
     */
    public getHeader(key: string): any {
        return this.headers[key];
    }

    public get type(): string {
        return this.headers.t;
    }

    public set type(value: string) {
        this.headers.t = value;
    }

    /**
     * @returns The participant id the message is to
     */
    public get to(): string {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return this.recipient!;
    }

    /**
     * @param value The participant id the message is to
     */
    public set to(value: string) {
        this.recipient = value;
    }

    /**
     * @returns The participant id the message is from
     */
    public get from(): string {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return this.sender!;
    }

    /**
     * @param value The participant id the message is from
     */
    public set from(value: string) {
        this.sender = value;
    }

    /**
     * @param value The expiry date of the message
     */
    public set expiryDate(value: number) {
        this.ttlMs = value;
    }

    /**
     * @returns The expiry date of the message
     */
    public get expiryDate(): number {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return this.ttlMs!;
    }

    /**
     * @param value The reply channel Id to return response messages to
     */
    public set replyChannelId(value: string) {
        this.headers.re = value;
    }

    /**
     * @returns The reply channel Id to return response messages to
     */
    public get replyChannelId(): string {
        return this.headers.re;
    }

    public set msgId(value: string) {
        this.headers.id = value;
    }
    public get msgId(): string {
        return this.headers.id;
    }

    public set compress(value: boolean) {
        this.isCompressed = value;
    }
    public get compress(): boolean {
        // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
        return this.isCompressed!;
    }

    /**
     * @param value The effort to be expended while delivering the message
     */
    public set effort(value: "NORMAL" | "BEST_EFFORT") {
        this.headers.ef = value;
    }

    /**
     * @returns The effort to be expended while delivering the message
     */
    public get effort(): "NORMAL" | "BEST_EFFORT" {
        return this.headers.ef;
    }
}

JoynrMessage.prototype._typeName = "joynr.JoynrMessage";

JoynrMessage.prototype.isTtlAbsolute = true;
JoynrMessage.prototype.encryptionCert = null;
JoynrMessage.prototype.signingCert = null;
JoynrMessage.prototype.signingKey = null;
JoynrMessage.prototype.isReceivedFromGlobal = false;
JoynrMessage.prototype.isLocalMessage = false;
JoynrMessage.prototype.isCompressed = false;

export = JoynrMessage;
