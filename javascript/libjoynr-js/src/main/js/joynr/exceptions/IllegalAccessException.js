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
let defaultSettings;

/**
 * @classdesc
 *
 * @summary
 * Constructor of IllegalAccessException object used for reporting
 * error conditions due to access restrictions that should be reported
 * back to consumer side.
 *
 * @constructor
 * @name IllegalAccessException
 *
 * @param {Object}
 *            [settings] the settings object for the constructor call
 * @param {String}
 *            [settings.detailMessage] message containing details
 *            about the error
 * @returns {IllegalAccessException}
 *            The newly created IllegalAccessException object
 */
function IllegalAccessException(settings) {
    if (!(this instanceof IllegalAccessException)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new IllegalAccessException(settings);
    }

    const joynrRuntimeException = new JoynrRuntimeException(settings);

    /**
     * Used for serialization.
     * @name IllegalAccessException#_typeName
     * @type String
     */
    UtilInternal.objectDefineProperty(this, "_typeName", "joynr.exceptions.IllegalAccessException");

    /**
     * See [constructor description]{@link IllegalAccessException}.
     * @name IllegalAccessException#detailMessage
     * @type String
     */
    this.detailMessage = undefined;

    UtilInternal.extend(this, defaultSettings, settings, joynrRuntimeException);
}

defaultSettings = {};

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.IllegalAccessException", IllegalAccessException);

IllegalAccessException.prototype = new Error();
IllegalAccessException.prototype.constructor = IllegalAccessException;
IllegalAccessException.prototype.name = "IllegalAccessException";

module.exports = IllegalAccessException;
