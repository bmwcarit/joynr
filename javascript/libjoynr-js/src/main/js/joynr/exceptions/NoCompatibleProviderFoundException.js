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
var TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
var Util = require("../util/UtilInternal");
var DiscoveryException = require("./DiscoveryException");
var LoggingManager = require("../system/LoggingManager");
var defaultSettings;

/**
 * @classdesc
 *
 * @summary
 * Constructor of NoCompatibleProviderFoundException object used for reporting
 * error conditions during discovery and arbitration when only providers
 * with incompatible versions are found. At least one such provider must
 * have been found, otherwise DiscoveryException will be used.
 *
 * @constructor
 * @name NoCompatibleProviderFoundException
 *
 * @param {Object}
 *            [settings] the settings object for the constructor call
 * @param {String}
 *            [settings.detailMessage] message containing details
 *            about the error
 * @param {String}
 *            [settings.interfaceName] the name of the interface
 * @param {Version[]}
 *            [settings.discoveredVersions] list of discovered
 *            but incompatible provider versions
 * @returns {NoCompatibleProviderFoundException}
 *            The newly created NoCompatibleProviderFoundException object
 */
function NoCompatibleProviderFoundException(settings) {
    if (!(this instanceof NoCompatibleProviderFoundException)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new NoCompatibleProviderFoundException(settings);
    }

    var log = LoggingManager.getLogger("joynr.exceptions.NoCompatibleProviderFoundException");
    var discoveryException = new DiscoveryException(settings);

    /**
     * Used for serialization.
     * @name NoCompatibleProviderFoundException#_typeName
     * @type String
     */
    Util.objectDefineProperty(this, "_typeName", "joynr.exceptions.NoCompatibleProviderFoundException");

    /**
     * See [constructor description]{@link NoCompatibleProviderFoundException}.
     * @name NoCompatibleProviderFoundException#detailMessage
     * @type String
     */
    this.detailMessage = undefined;

    /**
     * See [constructor description]{@link NoCompatibleProviderFoundException}.
     * @name NoCompatibleProviderFoundException#discoveredVersions
     * @type Version[]
     */
    this.discoveredVersions = undefined;

    /**
     * See [constructor description]{@link NoCompatibleProviderFoundException}.
     * @name NoCompatibleProviderFoundException#interfaceName
     * @type String
     */
    this.interfaceName = undefined;

    Util.extend(this, defaultSettings, settings, discoveryException);
}

defaultSettings = {};

TypeRegistrySingleton.getInstance().addType(
    "joynr.exceptions.NoCompatibleProviderFoundException",
    NoCompatibleProviderFoundException
);

NoCompatibleProviderFoundException.prototype = new Error();
NoCompatibleProviderFoundException.prototype.constructor = NoCompatibleProviderFoundException;
NoCompatibleProviderFoundException.prototype.name = "NoCompatibleProviderFoundException";

module.exports = NoCompatibleProviderFoundException;
