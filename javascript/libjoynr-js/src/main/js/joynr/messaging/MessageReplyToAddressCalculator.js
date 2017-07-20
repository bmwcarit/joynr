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

define(
        "joynr/messaging/MessageReplyToAddressCalculator",
        [ "joynr/messaging/JoynrMessage"
        ],
        function(JoynrMessage) {

            /**
             * @name MessageReplyToAddressCalculator
             * @constructor
             * @param {Object} settings the settings object for this constructor call
             * @param {Address} settings.replyToAddress the address the reply should be send to
             */
            function MessageReplyToAddressCalculator(settings) {
                var replyToAddress;

                var checkForExistingReplyToAddress =
                        function() {
                            if (replyToAddress === undefined) {
                                throw new Error(
                                        "MessageReplyToAddressCalculator: replyToAddress not specified!");
                            }
                        };

                /**
                 * Helper function allowing to share the serialized reply to address with the calculator after object creation
                 */
                this.setReplyToAddress = function(serializedAddress) {
                    replyToAddress = serializedAddress;
                    if (replyToAddress !== undefined) {
                        //disable check implementation
                        checkForExistingReplyToAddress = function() {};
                    }
                };

                this.setReplyTo =
                        function(message) {
                            var type = message.type;
                            if ((type !== undefined)
                                && (message.replyChannelId === undefined)
                                && ((type === JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST)
                                    || (type === JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST)
                                    || (type === JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST) || (type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST))) {
                                checkForExistingReplyToAddress();
                                message.replyChannelId = replyToAddress;
                            }
                        };

                if (settings.replyToAddress !== undefined) {
                    this.setReplyToAddress(settings.replyToAddress);
                }
            }

            return MessageReplyToAddressCalculator;

        });