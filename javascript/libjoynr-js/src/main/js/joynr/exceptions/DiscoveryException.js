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

class DiscoveryException extends JoynrRuntimeException {
    /**
     * Constructor of DiscoveryException object used for reporting
     * error conditions during discovery and arbitration.
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @returns {DiscoveryException} The newly created DiscoveryException object
     */
    constructor(settings) {
        super(settings);

        /**
         * Used for serialization.
         * @name DiscoveryException#_typeName
         * @type String
         */
        this._typeName = "joynr.exceptions.DiscoveryException";
        this.name = "DiscoveryException";
    }
}

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.DiscoveryException", DiscoveryException);

module.exports = DiscoveryException;
