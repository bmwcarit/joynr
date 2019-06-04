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
class JoynrException extends Error {
    /**
     * Constructor of JoynrException object used for reporting
     * error conditions. This serves as base class for the underlying
     * ApplicationException and JoynrRuntimeException.
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @returns {JoynrException} The newly created JoynrException object
     */
    constructor(settings = {}) {
        super();
        /**
         * Used for serialization.
         * @name JoynrException#_typeName
         * @type String
         */
        this._typeName = "joynr.exceptions.JoynrException";
        this.name = "JoynrException";

        /**
         * See [constructor description]{@link JoynrException}.
         * @name JoynrException#detailMessage
         * @type String
         */
        this.detailMessage = settings.detailMessage;
    }

    static _typeName = "joynr.exceptions.JoynrException";
}

module.exports = JoynrException;
