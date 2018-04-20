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
 * Constructor of DiscoveryException object used for reporting
 * error conditions during discovery and arbitration.
 *
 * @constructor
 * @name DiscoveryException
 *
 * @param {Object}
 *            [settings] the settings object for the constructor call
 * @param {String}
 *            [settings.detailMessage] message containing details
 *            about the error
 * @returns {DiscoveryException}
 *            The newly created DiscoveryException object
 */
function DiscoveryException(settings) {
    if (!(this instanceof DiscoveryException)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new DiscoveryException(settings);
    }

    const joynrRuntimeException = new JoynrRuntimeException(settings);

    /**
     * Used for serialization.
     * @name DiscoveryException#_typeName
     * @type String
     */
    UtilInternal.objectDefineProperty(this, "_typeName", "joynr.exceptions.DiscoveryException");

    /**
     * See [constructor description]{@link DiscoveryException}.
     * @name DiscoveryException#detailMessage
     * @type String
     */
    this.detailMessage = undefined;

    UtilInternal.extend(this, defaultSettings, settings, joynrRuntimeException);
}

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.DiscoveryException", DiscoveryException);

DiscoveryException.prototype = new Error();
DiscoveryException.prototype.constructor = DiscoveryException;
DiscoveryException.prototype.name = "DiscoveryException";

module.exports = DiscoveryException;
