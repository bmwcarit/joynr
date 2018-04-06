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
var Typing = require("../util/Typing");
var Util = require("../util/UtilInternal");
var JoynrRuntimeException = require("./JoynrRuntimeException");
var LoggingManager = require("../system/LoggingManager");
var defaultSettings;

/**
 * @classdesc
 *
 * @summary
 * Constructor of PublicationMissedException object used to report
 * when a publication has not been received within the expected
 * time period.
 *
 * @constructor
 * @name PublicationMissedException
 *
 * @param {Object}
 *            [settings] the settings object for the constructor call
 * @param {String}
 *            [settings.detailMessage] message containing details
 *            about the error
 * @param {String}
 *            [settings.subscriptionId] the id of the subscription
 * @returns {PublicationMissedException}
 *            The newly created PublicationMissedException object
 */
function PublicationMissedException(settings) {
    if (!(this instanceof PublicationMissedException)) {
        // in case someone calls constructor without new keyword (e.g. var c
        // = Constructor({..}))
        return new PublicationMissedException(settings);
    }

    var log = LoggingManager.getLogger("joynr.exceptions.PublicationMissedException");
    var runtimeException = new JoynrRuntimeException(settings);

    Typing.checkProperty(settings.subscriptionId, "String", "settings.subscriptionId");

    /**
     * Used for serialization.
     * @name PublicationMissedException#_typeName
     * @type String
     */
    Util.objectDefineProperty(this, "_typeName", "joynr.exceptions.PublicationMissedException");

    Util.extend(this, defaultSettings, settings, runtimeException);
}

defaultSettings = {};

TypeRegistrySingleton.getInstance().addType("joynr.exceptions.PublicationMissedException", PublicationMissedException);

PublicationMissedException.prototype = new Error();
PublicationMissedException.prototype.constructor = PublicationMissedException;
PublicationMissedException.prototype.name = "PublicationMissedException";

module.exports = PublicationMissedException;
