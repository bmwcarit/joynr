/*jslint es5: true, nomen: true, node: true */

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
var Typing = require('../../util/Typing');
module.exports = (function(Typing) {

    /**
     * @constructor WebMessagingAddress
     * @param {Object} settings the settings object for this constructor call
     * @param {Object} settings.window the default target window, the messages should be sent to
     * @param {String} settings.origin the default origin, the messages should be sent to
     */
    function WebMessagingAddress(settings) {
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.window, "Object", "settings.window");
        Typing.checkProperty(settings.origin, "String", "settings.origin");
        this._settings = settings;

        /**
         * @name WebMessagingAddress#getWindow
         * @function
         *
         * @returns {Window} the windows that should be addressed
         */
        WebMessagingAddress.prototype.getWindow = function getWindow() {
            return this._settings.window;
        };

        /**
         * @name WebMessagingAddress#getOrigin
         * @function
         *
         * @returns {String} the origin of the window that should be addressed
         * @see WebMessagingAddress#getWindow
         */
        WebMessagingAddress.prototype.getOrigin = function getOrigin() {
            return this._settings.origin;
        };
    }

    return WebMessagingAddress;

}(Typing));