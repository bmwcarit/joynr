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

var localStorage = (function() {
    var pub = {};

    var map = {};

    pub.setItem = function(key, value) {
        map[key] = value;
    };

    pub.getItem = function(key) {
        return map[key];
    };

    pub.removeItem = function(key) {
        delete map[key];
    };

    pub.clear = function() {
        map = {};
    };

    return pub;
}());