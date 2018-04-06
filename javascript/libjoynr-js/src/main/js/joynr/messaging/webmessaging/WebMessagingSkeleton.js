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
var Typing = require("../../util/Typing");
var Util = require("../../util/UtilInternal");

/**
 * @constructor WebMessagingSkeleton
 * @param {Object} settings the settings object for this constructor call
 * @param {Window} settings.window the window to register the event handler at
 */
function WebMessagingSkeleton(settings) {
    Typing.checkProperty(settings, "Object", "settings");

    if (settings.window === undefined) {
        throw new Error("WebMessagingSkeleton constructor parameter windows is undefined");
    }

    if (settings.window.addEventListener === undefined || settings.window.removeEventListener === undefined) {
        throw new Error(
            'WebMessagingSkeleton constructor parameter window does not provide the expected functions "addEventListener" and "removeEventListener"'
        );
    }

    var receiverCallbacks = [];
    var callbackFct = function(event) {
        Util.fire(receiverCallbacks, event.data);
    };

    settings.window.addEventListener("message", callbackFct);

    /**
     * Registers a listener for web messaging
     * @function WebMessagingSkeleton#registerListener
     *
     * @param {Function} listener the listener function receiving the messaging events events with the signature "function(joynrMessage) {..}"
     */
    this.registerListener = function registerListener(listener) {
        Typing.checkPropertyIfDefined(listener, "Function", "listener");

        receiverCallbacks.push(listener);
    };

    /**
     * Unregisters a listener for web messaging
     * @function WebMessagingSkeleton#unregisterListener
     *
     * @param {Function} listener the listener function receiving the messaging events events with the signature "function(joynrMessage) {..}"
     */
    this.unregisterListener = function unregisterListener(listener) {
        Typing.checkPropertyIfDefined(listener, "Function", "listener");

        Util.removeElementFromArray(receiverCallbacks, listener);
    };

    /**
     * @function WebMessagingSkeleton#shutdown
     */
    this.shutdown = function shutdown() {
        settings.window.removeEventListener("message", callbackFct);
    };
}

module.exports = WebMessagingSkeleton;
