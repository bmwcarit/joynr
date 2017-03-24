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

define("joynr/messaging/MessagingStubFactory", [
    "joynr/util/Typing",
    "joynr/util/UtilInternal",
    "joynr/system/LoggerFactory"
], function(Typing, Util, LoggerFactory) {

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
        var log = LoggerFactory.getLogger("joynr/messaging/MessagingStubFactory");
        var messagingStubFactories = settings.messagingStubFactories;

        /**
         * @name MessagingStubFactory#createMessagingStub
         * @function
         *
         * @param {MessagingStub} address the address to create a messaging stub for
         */
        this.createMessagingStub =
                function createMessagingStub(address) {
                    /*jslint nomen: true */
                    var className = address._typeName;
                    /*jslint nomen: false */
                    var factory = messagingStubFactories[className];

                    if (Util.checkNullUndefined(factory)) {
                        var errorMsg =
                                "Could not find a MessagingStubFactory for \""
                                    + className
                                    + "\" within messagingStubFactories ["
                                    + Object.keys(messagingStubFactories).join(",")
                                    + "]";
                        log.debug(errorMsg);
                        throw new Error(errorMsg);
                    }

                    return factory.build(address);
                };
    }

    return MessagingStubFactory;
});