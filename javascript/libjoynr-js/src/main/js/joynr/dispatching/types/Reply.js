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
var Util = require("../../util/UtilInternal");
var Typing = require("../../util/Typing");

/**
 * @name Reply
 * @constructor
 *
 * @param {Object}
 *            settings
 * @param {String}
 *            settings.requestReplyId
 * @param {Array}
 *            [settings.response] the response may be undefined
 * @param {Object}
 *            [settings.error] The exception object in case of request failure
 */
function Reply(settings) {
    var i;
    if (settings.response) {
        for (i = 0; i < settings.response.length; i++) {
            settings.response[i] = Util.ensureTypedValues(settings.response[i]);
        }
    }
    // must contain exactly one of the two alternatives
    if (!settings.response && !settings.error) {
        throw new Error("Reply object does neither contain response nor error");
    }
    if (settings.error && Array.isArray(settings.response) && settings.response.length > 0) {
        throw new Error("Reply object contains both response and error");
    }

    /**
     * @name Reply#requestReplyId
     * @type String
     */
    this.requestReplyId = settings.requestReplyId;
    /**
     * @name Reply#response
     * @type Array
     */
    this.response = settings.response;
    /**
     * @name Reply#error
     * @type Object
     */
    this.error = settings.error;

    /**
     * The joynr type name
     *
     * @name Reply#_typeName
     * @type String
     */
    Object.defineProperty(this, "_typeName", {
        value: "joynr.Reply",
        readable: true,
        writable: false,
        enumerable: true,
        configurable: false
    });

    return Object.freeze(this);
}

module.exports = Reply;
