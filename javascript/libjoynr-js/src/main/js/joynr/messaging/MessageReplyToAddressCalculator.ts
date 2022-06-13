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
import JoynrMessage from "./JoynrMessage";

type Address = any;

class MessageReplyToAddressCalculator {
    private checkExistingReplyAddress = true;
    private replyToAddress: Address;
    /**
     * @constructor
     * @param settings the settings object for this constructor call
     * @param settings.replyToAddress the address the reply should be send to
     */
    public constructor(settings: { replyToAddress?: Address }) {
        if (settings.replyToAddress !== undefined) {
            this.setReplyToAddress(settings.replyToAddress);
        }
    }

    private checkForExistingReplyToAddress(): void {
        if (this.checkExistingReplyAddress && this.replyToAddress === undefined) {
            throw new Error("MessageReplyToAddressCalculator: replyToAddress not specified!");
        }
    }

    /**
     * Helper function allowing to share the serialized reply to address with the calculator after object creation
     */
    public setReplyToAddress(serializedAddress: string): void {
        this.replyToAddress = serializedAddress;
        if (this.replyToAddress !== undefined) {
            //disable check implementation
            this.checkExistingReplyAddress = false;
        }
    }

    public setReplyTo(message: JoynrMessage): void {
        const type = message.type;
        if (
            type !== undefined &&
            message.replyChannelId === undefined &&
            (type === JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST ||
                type === JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST ||
                type === JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST ||
                type === JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST)
        ) {
            this.checkForExistingReplyToAddress();
            message.replyChannelId = this.replyToAddress;
        }
    }
}

export = MessageReplyToAddressCalculator;
