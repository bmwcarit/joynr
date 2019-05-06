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
const JoynrStorage = require("./JoynrPersist");
const LoggingManager = require("../joynr/system/LoggingManager");
const log = LoggingManager.getLogger("joynr.global.localStorageNode");

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
 *            settings.location optional, passed on to node-persist LocalStorage constructor
 */
const LocalStorageWrapper = function(settings) {
    settings = settings || {};
    //the local storage wrapper uses the optionally given location
    this._location = settings.location || "./localStorageStorage";
    this._storage = new JoynrStorage({
        dir: this._location
    });
    this._map = new Map();
    this._promiseChain = Promise.resolve();
    this._settings = settings;
};

LocalStorageWrapper.prototype = {
    setItem(key, value) {
        this._map.set(key, value);
        this._wrapFunction(this._storage.setItem.bind(this._storage), key, value);
    },
    getItem(key) {
        return this._map.get(key);
    },
    removeItem(key) {
        this._map.delete(key);
        this._wrapFunction(this._storage.removeItem.bind(this._storage), key);
    },
    clear() {
        this._map.clear();
        this._wrapFunction(this._storage.clear.bind(this._storage));
    },

    async init() {
        const storageData = await this._storage.init();

        if (this._settings.clearPersistency) {
            return await this._storage.clear();
        }
        for (let i = 0, length = storageData.length; i < length; i++) {
            const storageObject = storageData[i];
            if (storageObject && storageObject.key) {
                this._map.set(storageObject.key, storageObject.value);
            }
        }
    },

    async shutdown() {
        await this._promiseChain;
    },
    _wrapFunction(cb, ...args) {
        // eslint-disable-next-line promise/no-callback-in-promise
        this._promiseChain = this._promiseChain.then(() => cb(...args)).catch(e => {
            log.error(`failure executing ${cb} with args ${JSON.stringify(args)} error: ${e}`);
        });
    }
};

module.exports = LocalStorageWrapper;
