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
const Typing = require("../util/Typing");
const UtilInternal = require("../util/UtilInternal");
const defaultSettings = {};

class JoynrException {
    /**
     * @classdesc
     *
     * @summary
     * Constructor of JoynrException object used for reporting
     * error conditions. This serves as superobject for the underlying
     * ApplicationException and JoynrRuntimeException.
     *
     * @constructor
     * @name JoynrException
     *
     * @param {Object} [settings] the settings object for the constructor call
     * @param {String} [settings.detailMessage] message containing details
     *            about the error
     * @returns {JoynrException} The newly created JoynrException object
     */
    constructor(settings) {
        /**
         * Used for serialization.
         * @name JoynrException#_typeName
         * @type String
         */
        UtilInternal.objectDefineProperty(this, "_typeName", "joynr.exceptions.JoynrException");
        Typing.checkPropertyIfDefined(settings, "Object", "settings");
        if (settings && settings.detailMessage) {
            Typing.checkPropertyIfDefined(settings.detailMessage, "String", "settings.detailMessage");
            this.detailMessage = settings.detailMessage;
        } else {
            /**
             * See [constructor description]{@link JoynrException}.
             * @name JoynrException#detailMessage
             * @type String
             */
            this.detailMessage = undefined;
        }
        UtilInternal.extend(this, defaultSettings, settings);
    }
}

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.JoynrException", JoynrException);

JoynrException.prototype = new Error();
JoynrException.prototype.constructor = JoynrException;
JoynrException.prototype.name = "JoynrException";

module.exports = JoynrException;
