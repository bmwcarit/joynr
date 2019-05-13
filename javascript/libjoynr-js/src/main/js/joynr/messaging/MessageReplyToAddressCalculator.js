/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
const JoynrMessage = require("./JoynrMessage");

class MessageReplyToAddressCalculator {
    /**
     * @name MessageReplyToAddressCalculator
     * @constructor
     * @param {Object} settings the settings object for this constructor call
     * @param {Address} settings.replyToAddress the address the reply should be send to
     */
    constructor(settings) {
        this._replyToAddress = undefined;
        this._checkExistingReplyAddress = true;

        if (settings.replyToAddress !== undefined) {
            this.setReplyToAddress(settings.replyToAddress);
        }
    }

    _checkForExistingReplyToAddress() {
        if (this._checkExistingReplyAddress && this._replyToAddress === undefined) {
            throw new Error("MessageReplyToAddressCalculator: replyToAddress not specified!");
        }
    }

    /**
     * Helper function allowing to share the serialized reply to address with the calculator after object creation
     */
    setReplyToAddress(serializedAddress) {
        this._replyToAddress = serializedAddress;
        if (this._replyToAddress !== undefined) {
            //disable check implementation
            this._checkExistingReplyAddress = false;
        }
    }

    setReplyTo(message) {
        const type = message.type;
        if (
            type !== undefined &&
            message.replyChannelId === undefined &&
            (type === JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST ||
                type === JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
                type === JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST ||
                type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)
        ) {
            this._checkForExistingReplyToAddress();
            message.replyChannelId = this._replyToAddress;
        }
    }
}

module.exports = MessageReplyToAddressCalculator;
