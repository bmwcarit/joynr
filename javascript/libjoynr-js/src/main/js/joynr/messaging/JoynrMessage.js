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
var MESSAGE_CUSTOM_HEADER_PREFIX = "custom-";
var Util = require("../util/UtilInternal");
var uuid = require("../../lib/uuid-annotated");

var jmBase = uuid();
var jmIndex = 0;

/**
 * @name JoynrMessage
 * @constructor
 *
 * @param {Object}
 *            settings the settings object holding values for the JoynrMessage
 * @param {String}
 *            settings.type the message type as defined by JoynrMessage.JOYNRMESSAGE_TYPE_*
 */
function JoynrMessage(settings) {
    this.payload = settings.payload;
    this.headers = {};
    this.type = settings.type;
    this.msgId = settings.msgId || jmBase + "_" + jmIndex++;
    // TODO: check whether it makes sense to add more properties to the constructor
}

JoynrMessage.setSigningCallback = function(callback) {
    JoynrMessage.prototype.signingCallback = callback;
};

JoynrMessage.parseMessage = function(settings) {
    settings.headers = settings.headers || {};
    settings.headers.id = settings.headers.id || jmBase + "_" + jmIndex++;
    Util.setPrototypeOf(settings, JoynrMessage.prototype);
    return settings;
};

JoynrMessage.prototype._typeName = "joynr.JoynrMessage";

JoynrMessage.prototype.isTtlAbsolute = true;
JoynrMessage.prototype.encryptionCert = null;
JoynrMessage.prototype.signingCert = null;
JoynrMessage.prototype.signingKey = null;

/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY
 */
JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY = "o";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST
 */
JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST = "rq";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_REPLY
 */
JoynrMessage.JOYNRMESSAGE_TYPE_REPLY = "rp";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST
 */
JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST = "arq";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST
 */
JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST = "mrq";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST
 */
JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST = "brq";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY
 */
JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY = "srp";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION
 */
JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION = "p";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST
 */
JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST = "m";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP
 */
JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP = "sst";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID
 */
JoynrMessage.JOYNRMESSAGE_HEADER_CREATOR_USER_ID = "creator";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_HEADER_EFFORT
 */
JoynrMessage.JOYNRMESSAGE_HEADER_EFFORT = "effort";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID
 */
JoynrMessage.JOYNRMESSAGE_HEADER_REPLY_CHANNELID = "replyChannelId";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE
 */
JoynrMessage.JOYNRMESSAGE_HEADER_CONTENT_TYPE = "contentType";
/**
 * @static
 * @readonly
 * @type String
 * @name JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID
 */
JoynrMessage.JOYNRMESSAGE_HEADER_TO_PARTICIPANT_ID = "to";

/**
 * @name JoynrMessage#setCustomHeaders
 * @function
 * @param {Object}
 *            a map containing key/value pairs of headers to be set as custom
 *            headers. The keys will be added to the header field with the prefix
 *            MESSAGE_CUSTOM_HEADER_PREFIX
 * @returns {JoynrMessage}
 */
Object.defineProperty(JoynrMessage.prototype, "setCustomHeaders", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: function(customHeaders) {
        var headerKey;
        for (headerKey in customHeaders) {
            if (customHeaders.hasOwnProperty(headerKey)) {
                this.headers[MESSAGE_CUSTOM_HEADER_PREFIX + headerKey] = customHeaders[headerKey];
            }
        }
        return this;
    }
});

/**
 * @name JoynrMessage#getCustomHeaders
 * @function
 * @returns {Object} customHeader object containing all headers that begin with the
 *          prefix MESSAGE_CUSTOM_HEADER_PREFIX
 */
Object.defineProperty(JoynrMessage.prototype, "getCustomHeaders", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: function() {
        var headerKey,
            trimmedKey,
            customHeaders = {};
        for (headerKey in this.headers) {
            if (
                this.headers.hasOwnProperty(headerKey) &&
                headerKey.substr(0, MESSAGE_CUSTOM_HEADER_PREFIX.length) === MESSAGE_CUSTOM_HEADER_PREFIX
            ) {
                trimmedKey = headerKey.substr(MESSAGE_CUSTOM_HEADER_PREFIX.length);

                customHeaders[trimmedKey] = this.headers[headerKey];
            }
        }
        return customHeaders;
    }
});

/**
 * @name JoynrMessage#setHeader
 * @function
 *
 * @param {String}
 *            key is one of the header keys defined in JoynrMessagingDefines
 * @param {Any}
 *            value of the header
 * @returns {JoynrMessage}
 */
Object.defineProperty(JoynrMessage.prototype, "setHeader", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: function(key, value) {
        this.headers[key] = value;
        return this;
    }
});

/**
 * @name JoynrMessage#getHeader
 * @function
 *
 * @param {String}
 *            key is one of the header keys defined in JoynrMessagingDefines
 * @returns {Object}
 */
Object.defineProperty(JoynrMessage.prototype, "getHeader", {
    enumerable: false,
    configurable: false,
    writable: false,
    value: function(key) {
        return this.headers[key];
    }
});

JoynrMessage.prototype.payload = undefined;

Object.defineProperty(JoynrMessage.prototype, "type", {
    set: function(value) {
        this.headers.t = value;
    },
    get: function() {
        return this.headers.t;
    }
});

/**
 * The participant id the message is to
 *
 * @name JoynrMessage#to
 * @type String
 */
Object.defineProperty(JoynrMessage.prototype, "to", {
    set: function(value) {
        this.recipient = value;
    },
    get: function() {
        return this.recipient;
    }
});

/**
 * The participant id the message is from
 *
 * @name JoynrMessage#from
 * @type String
 */
Object.defineProperty(JoynrMessage.prototype, "from", {
    set: function(value) {
        this.sender = value;
    },
    get: function() {
        return this.sender;
    }
});

/**
 * The expiry date of the message
 *
 * @name JoynrMessage#expiryDate
 * @type String
 */
Object.defineProperty(JoynrMessage.prototype, "expiryDate", {
    set: function(value) {
        this.ttlMs = value;
    },
    get: function() {
        return this.ttlMs;
    }
});

JoynrMessage.prototype.creator = undefined; // TODO: check for some reason creator was basically unused

/**
 * The reply channel Id to return response messages to
 *
 * @name JoynrMessage#replyChannelId
 * @type String
 */
Object.defineProperty(JoynrMessage.prototype, "replyChannelId", {
    set: function(value) {
        this.headers.re = value;
    },
    get: function() {
        return this.headers.re;
    }
});

JoynrMessage.prototype.isReceivedFromGlobal = false;
// writing the variable on the prototype acts as a default value

JoynrMessage.prototype.isLocalMessage = false;
// no defineProperty for localMessage since the variable may simply be used

Object.defineProperty(JoynrMessage.prototype, "msgId", {
    set: function(value) {
        this.headers.id = value;
    },
    get: function() {
        return this.headers.id;
    }
});

JoynrMessage.prototype.isCompressed = false;

Object.defineProperty(JoynrMessage.prototype, "compress", {
    set: function(value) {
        this.isCompressed = value;
    },
    get: function() {
        return this.isCompressed;
    }
});

/**
 * The effort to be expent while delivering the message
 *
 * @name JoynrMessage#effort
 * @type String
 */

Object.defineProperty(JoynrMessage.prototype, "effort", {
    set: function(value) {
        this.headers.ef = value;
    },
    get: function() {
        return this.headers.ef;
    }
});

module.exports = JoynrMessage;
