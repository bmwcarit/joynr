/*eslint no-useless-escape: "off"*/
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
const defaultMessagingSettings = require("../start/settings/defaultMessagingSettings");
const LoggingManager = require("../system/LoggingManager");
const UtilInternal = require("../util/UtilInternal");
const MessagingQosEffort = require("./MessagingQosEffort");

const log = LoggingManager.getLogger("joynr/messaging/MessagingQos");
const defaultSettings = {
    ttl: 60000,
    customHeaders: {},
    encrypt: false,
    compress: false
};

/**
 * Constructor of MessagingQos object that is used in the generation of proxy objects
 * @constructor
 * @name MessagingQos
 *
 * @param {Object} [settings] the settings object for the constructor call
 * @param {Number} [settings.ttl] Roundtrip timeout for rpc requests, if missing default value is 60 seconds
 * @param {Boolean} [settings.encrypt] Specifies whether messages will be sent encrypted
 * @param {Boolean} [settings.compress] Specifies whether messages will be sent compressed
 * @param {MessagingQosEffort} [settings.effort] effort to expend on ensuring message delivery
 *
 * @returns {MessagingQos} a messaging Qos Object
 */
function MessagingQos(settings) {
    let errorMsg;

    if (!(this instanceof MessagingQos)) {
        // in case someone calls constructor without new keyword (e.g. var c = Constructor({..}))
        return new MessagingQos(settings);
    }

    settings = UtilInternal.extend({}, defaultSettings, settings);

    if (!MessagingQosEffort.isValid(settings.effort)) {
        settings.effort = MessagingQosEffort.NORMAL;
    }

    /**
     * The time to live for messages
     *
     * @name MessagingQos#ttl
     * @type Number
     */
    if (settings.ttl > defaultMessagingSettings.MAX_MESSAGING_TTL_MS) {
        this.ttl = defaultMessagingSettings.MAX_MESSAGING_TTL_MS;
        errorMsg = `Error in MessageQos. Max allowed ttl: ${
            defaultMessagingSettings.MAX_MESSAGING_TTL_MS
        }. Passed ttl: ${settings.ttl}`;
        log.warn(errorMsg);
    } else {
        this.ttl = settings.ttl;
    }

    /**
     * custom message headers
     *
     * @name MessagingQos#customHeaders
     * @type Object
     */
    this.customHeaders = settings.customHeaders;

    /**
     * messaging qos effort
     *
     * @name MessagingQos#effort
     * @type MessagingQosEffort
     */
    this.effort = settings.effort;

    /**
     * encrypt
     *
     * @name MessagingQos#encrypt
     * @type Boolean
     */
    this.encrypt = settings.encrypt;

    if (settings.encrypt !== true && settings.encrypt !== false) {
        throw new Error("encrypt may only contain a boolean");
    }

    /**
     * compress
     *
     * @name MessagingQos#compress
     * @type Boolean
     */
    this.compress = settings.compress;

    if (settings.compress !== true && settings.compress !== false) {
        throw new Error("compress may only contain a boolean");
    }
}

/**
 *
 * @param {String} key
 *            may contain ascii alphanumeric or hyphen.
 * @param {String} value
 *            may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark, hyphen,
 *            dot, star, forward slash and back slash.
 */
function checkKeyAndValue(key, value) {
    const keyPattern = /^[a-zA-Z0-9\-]*$/;
    const valuePattern = /^[a-zA-Z0-9 ;:,+&\?\-\.\*\/\\]*$/;
    const keyOk = keyPattern.test(key);
    const valueOk = valuePattern.test(value);
    if (!keyOk) {
        throw new Error("custom header key may only contain alphanumeric characters");
    }
    if (!valueOk) {
        throw new Error("custom header value contains illegal character. See JSDoc for allowed characters");
    }
    return true;
}

/**
 * @name MessagingQos#putCustomHeader
 * @function
 *
 * @param {String} key
 *            may contain ascii alphanumeric or hyphen.
 * @param {String} value
 *            may contain alphanumeric, space, semi-colon, colon, comma, plus, ampersand, question mark, hyphen,
 *            dot, star, forward slash and back slash.
 * @returns {JoynrMessage}
 */
Object.defineProperty(MessagingQos.prototype, "putCustomMessageHeader", {
    enumerable: false,
    configurable: false,
    writable: false,
    value(key, value) {
        checkKeyAndValue(key, value);
        this.customHeaders[key] = value;
    }
});

/**
 * @name MessagingQos.DEFAULT_TTL
 * @type Number
 * @default 60000
 * @static
 * @readonly
 */
MessagingQos.DEFAULT_TTL = defaultSettings.ttl;

/**
 * @name MessagingQos.DEFAULT_ENCRYPT
 * @type Boolean
 * @default false
 * @static
 * @readonly
 */
MessagingQos.DEFAULT_ENCRYPT = defaultSettings.encrypt;

/**
 * @name MessagingQos.DEFAULT_COMPRESS
 * @type Boolean
 * @default false
 * @static
 * @readonly
 */
MessagingQos.DEFAULT_COMPRESS = defaultSettings.compress;

module.exports = MessagingQos;
