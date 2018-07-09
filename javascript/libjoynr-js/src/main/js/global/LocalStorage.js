/*global localStorage*/
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
 * LocalStorage
 *
 * @returns constructor for a localStorage object
 */
const Typing = require("../joynr/util/Typing");
const Promise = require("./Promise");

/**
 * constructor for a localStorage object
 * @param {Object}
 *            settings the settings object
 * @param {Boolean}
 *            settings.clearPersistency localStorage is cleared if set to true
 *
 * @constructor LocalStorage
 */
const LocalStorage = function(settings) {
    settings = settings || {};
    Typing.checkPropertyIfDefined(settings.clearPersistency, "Boolean", "settings.clearPersistency");
    if (settings.clearPersistency) {
        localStorage.clear();
    }
};

LocalStorage.prototype = {
    setItem: (key, value) => {
        localStorage.setItem(key, value);
    },
    getItem(key) {
        return localStorage.getItem(key);
    },
    removeItem(key) {
        localStorage.removeItem(key);
    },
    clear() {
        localStorage.clear();
    },
    init() {
        return Promise.resolve();
    },
    shutdown() {
        return Promise.resolve();
    }
};

module.exports = LocalStorage;
