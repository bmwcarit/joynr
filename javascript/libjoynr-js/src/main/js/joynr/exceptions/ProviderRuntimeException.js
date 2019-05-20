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
const TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
const JoynrRuntimeException = require("./JoynrRuntimeException");

class ProviderRuntimeException extends JoynrRuntimeException {
    /**
     * Constructor of ProviderRuntimeException object used for reporting
     * generic error conditions on the provider side that should be
     * transmitted back to consumer side.
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @returns {ProviderRuntimeException} The newly created ProviderRuntimeException object
     */
    constructor(settings = {}) {
        super(settings);

        /**
         * Used for serialization.
         * @name ProviderRuntimeException#_typeName
         * @type String
         */
        this._typeName = "joynr.exceptions.ProviderRuntimeException";
        this.name = "ProviderRuntimeException";
    }
}

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.ProviderRuntimeException", ProviderRuntimeException);

module.exports = ProviderRuntimeException;
