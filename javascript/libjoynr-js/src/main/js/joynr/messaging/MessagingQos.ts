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
/*eslint no-useless-escape: "off"*/
import defaultMessagingSettings from "../start/settings/defaultMessagingSettings";
import LoggingManager from "../system/LoggingManager";
import * as UtilInternal from "../util/UtilInternal";
import * as MessagingQosEffort from "./MessagingQosEffort";

const log = LoggingManager.getLogger("joynr/messaging/MessagingQos");
const defaultSettings = {
    ttl: 60000,
    customHeaders: {},
    encrypt: false,
    compress: false
};

namespace MessagingQos {
    export interface Settings {
        ttl: number;
        encrypt: boolean;
        compress: boolean;
        effort: MessagingQosEffort.NORMAL | MessagingQosEffort.BEST_EFFORT;
        customHeaders: Record<string, any>;
    }
}

class MessagingQos {
    /**
     * @name MessagingQos.DEFAULT_COMPRESS
     * @type Boolean
     * @default false
     * @static
     * @readonly
     */
    public static DEFAULT_COMPRESS = defaultSettings.compress;

    /**
     * @name MessagingQos.DEFAULT_ENCRYPT
     * @type Boolean
     * @default false
     * @static
     * @readonly
     */
    public static DEFAULT_ENCRYPT = defaultSettings.encrypt;

    /**
     * @name MessagingQos.DEFAULT_TTL
     * @type Number
     * @default 60000
     * @static
     * @readonly
     */
    public static DEFAULT_TTL = defaultSettings.ttl;
    public compress: boolean;
    public encrypt: boolean;
    public effort: MessagingQosEffort.NORMAL | MessagingQosEffort.BEST_EFFORT;

    /**
     * custom message headers
     *
     * @name MessagingQos#customHeaders
     * @type Object
     */
    public customHeaders?: Record<string, any>;

    /**
     * The time to live for messages in ms
     */
    public ttl!: number;

    /**
     * Constructor of MessagingQos object that is used in the generation of proxy objects
     *
     * @param [settings] the settings object for the constructor call
     * @param [settings.ttl] Roundtrip timeout for rpc requests, if missing default value is 60 seconds
     * @param [settings.encrypt] Specifies whether messages will be sent encrypted
     * @param [settings.compress] Specifies whether messages will be sent compressed
     * @param [settings.effort] effort to expend on ensuring message delivery
     */
    public constructor(settings?: Partial<MessagingQos.Settings>) {
        const augmentedSettings: MessagingQos.Settings = UtilInternal.extend({}, defaultSettings, settings);

        if (!MessagingQosEffort.isValid(augmentedSettings.effort)) {
            augmentedSettings.effort = MessagingQosEffort.NORMAL;
        }

        if (augmentedSettings.ttl > defaultMessagingSettings.MAX_MESSAGING_TTL_MS) {
            this.ttl = defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
            const errorMsg = `Error in MessageQos. Max allowed ttl: ${
                defaultMessagingSettings.MAX_MESSAGING_TTL_MS
            }. Passed ttl: ${augmentedSettings.ttl}`;
            log.warn(errorMsg);
        } else {
            this.ttl = augmentedSettings.ttl;
        }

        this.customHeaders = augmentedSettings.customHeaders;
        this.effort = augmentedSettings.effort;
        this.encrypt = augmentedSettings.encrypt;
        this.compress = augmentedSettings.compress;
    }

    /**
     * @param key may contain ascii alphanumeric or hyphen.
     * @param value may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand,
     *  question mark, hyphen, dot, star, forward slash and back slash.
     */
    public putCustomMessageHeader(key: string, value: string): void {
        const keyPattern = /^[a-zA-Z0-9\-]*$/;
        const valuePattern = /^[a-zA-Z0-9 ;:,+&\?\-\.\*\/\\_]*$/;
        const keyOk = keyPattern.test(key);
        const valueOk = valuePattern.test(value);
        if (!keyOk) {
            throw new Error("custom header key may only contain alphanumeric characters");
        }
        if (!valueOk) {
            throw new Error("custom header value contains illegal character. See JSDoc for allowed characters");
        }
        this.customHeaders = this.customHeaders || {};
        this.customHeaders[key] = value;
    }
}

export = MessagingQos;
