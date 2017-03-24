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

define("intertab/comm/WebMessageReceiver", [], function() {
    /**
     * @param {Object} settings the settings object for this constructor call
     * @param {Window} settings.window the window to register the event handler at
     * @param {Function} [settings.receive] the function receiving all events with the signature "function(event) {..}"
     */
    function WebMessageReceiver(settings) {
        if (settings === undefined || settings.window === undefined) {
            throw new Error("WebMessageReceiver(): provided settings object " + settings + " does not contain the required parameter \"window\"");
        }

        function registerReceive(receive) {
            settings.window.addEventListener('message', receive);
        }

        if (settings && settings.receive) {
            registerReceive(settings.receive);
        }

        /**
         * Registers a receiver function for web messaging
         *
         * @param {Function} receive the function receiving all events with the signature "function(event) {..}"
         */
        this.registerReceive = registerReceive;
    }

    return WebMessageReceiver;
});
