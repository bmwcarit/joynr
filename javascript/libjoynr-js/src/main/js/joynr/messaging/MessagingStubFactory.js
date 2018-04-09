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
const Typing = require("../util/Typing");
const Util = require("../util/UtilInternal");
const LoggingManager = require("../system/LoggingManager");

const log = LoggingManager.getLogger("joynr/messaging/MessagingStubFactory");
/**
 * @name MessagingStubFactory
 * @constructor
 *
 * @param {Object} settings
 * @param {Object} settings.messagingStubFactories a hash mapping addresses to the equivalent message sender
 * @param {MessagingStubFactory} settings.messagingStubFactories.KEY the key of the map is the className of the address provided in createMessagingStub, the value is the concrete MessagingStubFactory
 * @param {Function} settings.messagingStubFactories.KEY.build the factory method of the
 */
function MessagingStubFactory(settings) {
    this._messagingStubFactories = settings.messagingStubFactories;
}

/**
 * @name MessagingStubFactory#createMessagingStub
 * @function
 *
 * @param {MessagingStub} address the address to create a messaging stub for
 */
MessagingStubFactory.prototype.createMessagingStub = function createMessagingStub(address) {
    const className = address._typeName;
    const factory = this._messagingStubFactories[className];

    if (Util.checkNullUndefined(factory)) {
        const errorMsg =
            'Could not find a MessagingStubFactory for "' +
            className +
            '" within messagingStubFactories [' +
            Object.keys(this._messagingStubFactories).join(",") +
            "]";
        log.debug(errorMsg);
        throw new Error(errorMsg);
    }

    return factory.build(address);
};

module.exports = MessagingStubFactory;
