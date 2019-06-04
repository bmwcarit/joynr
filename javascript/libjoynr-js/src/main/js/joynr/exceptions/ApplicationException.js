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
const Typing = require("../util/Typing");
const JoynrException = require("./JoynrException");
const defaultMessage = "This is an application exception.";

class ApplicationException extends JoynrException {
    /**
     * Constructor of ApplicationException object used for reporting
     * error conditions from method implementations. The settings.error
     * object must be filled with _typeName and name as serialization
     * of an enum object of the matching error enum type defined in
     * Franca.
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param settings.error the error enum to be reported
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @returns {ApplicationException} The newly created ApplicationException object
     */
    constructor(settings = {}) {
        settings.detailMessage = settings.detailMessage || defaultMessage;
        super(settings);

        /**
         * Used for serialization.
         * @name ApplicationException#_typeName
         * @type String
         */
        this._typeName = "joynr.exceptions.ApplicationException";
        this.name = "ApplicationException";

        this.error = settings.error;

        if (settings && settings.error) {
            Typing.checkProperty(settings.error.name, "String", "settings.error.name");
            Typing.checkProperty(settings.error.value, ["String", "Number"], "settings.error.value");
        }
    }

    static _typeName = "joynr.exceptions.ApplicationException";
}

module.exports = ApplicationException;
