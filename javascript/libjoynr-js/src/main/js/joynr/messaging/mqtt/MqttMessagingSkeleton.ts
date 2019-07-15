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
import MqttAddress from "../../../generated/joynr/system/RoutingTypes/MqttAddress";
import SharedMqttClient = require("./SharedMqttClient");
import MessageRouter = require("../routing/MessageRouter");
import JoynrMessage = require("../JoynrMessage");

class MqttMessagingSkeleton {
    private client: SharedMqttClient;
    private multicastSubscriptionCount: any = {};
    /**
     * @constructor MqttMessagingSkeleton
     * @param settings
     * @param settings.client the mqtt client to be used to transmit messages
     * @param settings.messageRouter the message router
     * @param settings.address own address of joynr client
     */
    public constructor(settings: { client: SharedMqttClient; messageRouter: MessageRouter; address: MqttAddress }) {
        this.client = settings.client;

        this.client.onmessage = function(_topic: string, message: JoynrMessage) {
            message.isReceivedFromGlobal = true;
            settings.messageRouter.route(message);
        };

        this.client.subscribe(`${settings.address.topic}/#`);
    }

    private translateWildcard(multicastId: string): string {
        if (multicastId.match(/[\w\W]*\/[*]$/)) {
            return multicastId.replace(/\/\*/g, "/#");
        }
        return multicastId;
    }

    public registerMulticastSubscription(multicastId: string): void {
        if (this.multicastSubscriptionCount[multicastId] === undefined) {
            this.multicastSubscriptionCount[multicastId] = 0;
        }
        this.client.subscribe(this.translateWildcard(multicastId));
        this.multicastSubscriptionCount[multicastId] = this.multicastSubscriptionCount[multicastId] + 1;
    }

    public unregisterMulticastSubscription(multicastId: string): void {
        let subscribersCount = this.multicastSubscriptionCount[multicastId];
        if (subscribersCount !== undefined) {
            subscribersCount--;
            if (subscribersCount === 0) {
                this.client.unsubscribe(this.translateWildcard(multicastId));
                delete this.multicastSubscriptionCount[multicastId];
            } else {
                this.multicastSubscriptionCount[multicastId] = subscribersCount;
            }
        }
    }

    public shutdown(): void {
        this.client.close();
    }
}

export = MqttMessagingSkeleton;
