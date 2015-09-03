/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
define([
    "global/LocalStorageNode",
    "joynr/util/uuid"
], function(LocalStorage, uuid) {
    /**
     * LocalStorage constructor
     *
     * @param {String}
     *            newLocation optional, passed on to node-localstorage LocalStorage constructor
     * @param {Number}
     *            quota optional, passed on to node-localstorage LocalStorage constructor
     */
    var LocalStorageWrapper = function(newLocation, quota) {
        var location = newLocation || "${project.build.directory}/LocalStorage-" + uuid();
        return new LocalStorage(location, quota);
    };
    return LocalStorageWrapper;
});