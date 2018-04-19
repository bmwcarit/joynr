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
const TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
const UtilInternal = require("../util/UtilInternal");
const JoynrRuntimeException = require("./JoynrRuntimeException");
const defaultSettings = {};

/**
 * @classdesc
 *
 * @summary
 * Constructor of ProviderRuntimeException object used for reporting
 * generic error conditions on the provider side that should be
 * transmitted back to consumer side.
 *
 * @constructor
 * @name ProviderRuntimeException
 *
 * @param {Object}
 *            [settings] the settings object for the constructor call
 * @param {String}
 *            [settings.detailMessage] message containing details
 *            about the error
 * @returns {ProviderRuntimeException}
 *            The newly created ProviderRuntimeException object
 */
function ProviderRuntimeException(settings) {
    if (!(this instanceof ProviderRuntimeException)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new ProviderRuntimeException(settings);
    }

    const runtimeException = new JoynrRuntimeException(settings);

    /**
     * Used for serialization.
     * @name ProviderRuntimeException#_typeName
     * @type String
     */
    UtilInternal.objectDefineProperty(this, "_typeName", "joynr.exceptions.ProviderRuntimeException");

    UtilInternal.extend(this, defaultSettings, settings, runtimeException);
}

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.ProviderRuntimeException", ProviderRuntimeException);

ProviderRuntimeException.prototype = new Error();
ProviderRuntimeException.prototype.constructor = ProviderRuntimeException;
ProviderRuntimeException.prototype.name = "ProviderRuntimeException";

module.exports = ProviderRuntimeException;
