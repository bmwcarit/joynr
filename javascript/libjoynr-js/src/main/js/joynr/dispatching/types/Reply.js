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
    // must contain exactly one of the two alternatives
    if (!settings.response && !settings.error) {
        throw new Error("Reply object does neither contain response nor error");
    }
    if (settings.error && Array.isArray(settings.response) && settings.response.length > 0) {
        throw new Error("Reply object contains both response and error");
    }

    /**
     * The joynr type name
     *
     * @name Reply#_typeName
     * @type String
     */
    Object.defineProperty(settings, "_typeName", {
        value: "joynr.Reply",
        enumerable: true
    });

    return settings;
}

exports.create = Reply;
