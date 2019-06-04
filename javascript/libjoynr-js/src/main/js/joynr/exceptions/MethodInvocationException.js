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
const JoynrRuntimeException = require("./JoynrRuntimeException");

class MethodInvocationException extends JoynrRuntimeException {
    /**
     * Constructor of MethodInvocationException object used for reporting
     * error conditions when invoking a method (e.g. method does not
     * exist or no method with matching signature found etc.) that should
     * be transmitted back to consumer side.
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {Version} [settings.providerVersion] the version of the provider
     *            which could not handle the method invocation
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @returns {MethodInvocationException} The newly created MethodInvocationException object
     */
    constructor(settings = {}) {
        super();

        /**
         * Used for serialization.
         * @name MethodInvocationException#_typeName
         * @type String
         */
        this._typeName = "joynr.exceptions.MethodInvocationException";
        this.name = "MethodInvocationException";

        /**
         * The provider version information
         * @name MethodInvocationException#providerVersion
         * @type Version
         */
        this.providerVersion = settings.providerVersion;
        if (settings) {
            Typing.checkProperty(settings, "Object", "settings");
            Typing.checkPropertyIfDefined(settings.providerVersion, "Version", "settings.providerVersion");
        }
    }

    static _typeName = "joynr.exceptions.MethodInvocationException";
}

module.exports = MethodInvocationException;
