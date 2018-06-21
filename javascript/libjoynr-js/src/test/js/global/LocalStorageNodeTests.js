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
/**
 * node wrapper for LocalStorage for testing
 *
 * Sets the default storage path of LocalStorage to a sub-folder of the build directory with an uuid
 * appended to the folder name.
 *
 * @returns constructor for a localStorage object
 */
const LocalStorage = require("../../../main/js/global/LocalStorageNode");
const uuid = require("../../../main/js/joynr/util/uuid");
/**
 * LocalStorage constructor (node wrapper for LocalStorage)
 * @constructor LocalStorageWrapper
 * @classdesc node wrapper for LocalStorage
 *
 * @param {Object}
 *            settings the settings object
 * @param {Boolean}
 *            settings.clearPersistency localStorage is cleared if set to true
 * @param {String}
 *            settings.location optional, passed on to node-localstorage LocalStorage constructor
 * @param {Number}
 *            settings.quota optional, passed on to node-localstorage LocalStorage constructor
 */
const LocalStorageWrapper = function(settings) {
    settings = settings || {};
    settings.location = settings.location || `localStorageTestResults/LocalStorage-${uuid()}`;
    return new LocalStorage(settings);
};
module.exports = LocalStorageWrapper;
