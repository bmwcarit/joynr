/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
 * MemoryStorage has the same Interface as LocalStorage and is used when persistency is disabled,
 * but the logic relies on the persistency.
 * @constructor
 */
const MemoryStorage = function() {
    this.map = {};
};

MemoryStorage.prototype.setItem = function(key, value) {
    this.map[key] = value;
};

MemoryStorage.prototype.getItem = function(key) {
    return this.map[key];
};

MemoryStorage.prototype.removeItem = function(key) {
    delete this.map[key];
};

MemoryStorage.prototype.clear = function() {
    this.map = {};
};

module.exports = MemoryStorage;
