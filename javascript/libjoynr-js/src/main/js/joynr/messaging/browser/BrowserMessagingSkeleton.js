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
const JoynrMessage = require("../JoynrMessage");
const Typing = require("../../util/Typing");
const UtilInternal = require("../../util/UtilInternal");
const JSONSerializer = require("../../util/JSONSerializer");
const LoggingManager = require("../../system/LoggingManager");

/**
 * @constructor BrowserMessagingSkeleton
 *
 * @param {Object} settings
 * @param {WebMessagingSkeleton} settings.webMessagingSkeleton a web messaging skeleton receiving web messages
 */
function BrowserMessagingSkeleton(settings) {
    const log = LoggingManager.getLogger("joynr/messaging/browser/BrowserMessagingSkeleton");
    Typing.checkProperty(settings, "Object", "settings");
    Typing.checkProperty(settings.webMessagingSkeleton, Object, "settings.webMessagingSkeleton");

    const that = this;
    this.receiverCallbacks = [];

    function webMessagingSkeletonListener(message) {
        if (message !== undefined) {
            const joynrMessage = new JoynrMessage(message);

            UtilInternal.fire(that.receiverCallbacks, joynrMessage);
        } else {
            log.warn(`message with content "${JSONSerializer.stringify(message)}" could not be processed`);
        }
    }

    settings.webMessagingSkeleton.registerListener(webMessagingSkeletonListener);
}

/**
 * Registers the listener function
 *
 * @function BrowserMessagingSkeleton#registerListener
 *
 * @param {Function} listener a listener function that should be added and should receive messages
 */
BrowserMessagingSkeleton.prototype.registerListener = function(listener) {
    Typing.checkProperty(listener, "Function", "listener");

    this.receiverCallbacks.push(listener);
};

/**
 * Unregisters the listener function
 *
 * @function BrowserMessagingSkeleton#unregisterListener
 *
 * @param {Function} listener the listener function that should re removed and shouldn't receive messages any more
 */
BrowserMessagingSkeleton.prototype.unregisterListener = function(listener) {
    Typing.checkProperty(listener, "Function", "listener");

    UtilInternal.removeElementFromArray(this.receiverCallbacks, listener);
};

module.exports = BrowserMessagingSkeleton;
