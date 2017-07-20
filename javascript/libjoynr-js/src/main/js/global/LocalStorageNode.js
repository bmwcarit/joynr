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
 * @returns constructor for a localStorage object
 */
define([
    "node-localstorage",
    "joynr/util/Typing"
], function(LocalStorage, Typing) {
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
    var LocalStorageWrapper =
            function(settings) {
                settings = settings || {};
                //the local storage wrapper uses the optionally given location
                var location = settings.location || "./localStorageStorage";
                var localStorage = new LocalStorage.LocalStorage(location, settings.quota);
                Typing.checkPropertyIfDefined(
                        settings.clearPersistency,
                        "Boolean",
                        "settings.clearPersistency");
                if (settings.clearPersistency) {
                    localStorage.clear();
                }
                return localStorage;
            };
    return LocalStorageWrapper;
});