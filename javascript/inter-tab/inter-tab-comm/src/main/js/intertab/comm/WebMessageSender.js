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

define("intertab/comm/WebMessageSender", [], function() {
    /**
     * @param {Object} settings the settings object for this constructor call
     * @param {Function} settings.defaultWindow optional default target window, the messages should be sent to
     */
    function WebMessageSender(settings) {

        var defaultWindow;

        if (settings !== undefined) {
            defaultWindow = settings.defaultWindow;
        }

        /**
         * Sends messages to the default or the provided window
         *
         * @param {Object} settings the settings object for this function call
         * @param {?} settings.message the message object to be sent
         * @param {Window} [settings.origin] the origin of the target window for the postMessage call, default is the own origin (location.origin)
         * @param {Window} [settings.targetWindow] if another targetWindow shall be used instead of the defaultWindow provided in the constructor
         */
        this.postMessage = function(settings) {
            var win = settings.targetWindow || defaultWindow;
            if (win === undefined){
                throw new Error("WebMessageSender.postMessage: parameter \"settings.targetWindow\" is missing");
            }
            if (settings.message === undefined){
                throw new Error("WebMessageSender.postMessage: parameter \"settings.message\" is missing");
            }
            win.postMessage(settings.message, settings.origin || location.origin);
        };
    }

    return WebMessageSender;
});