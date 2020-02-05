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
import * as MqttAddress from "../../../generated/joynr/system/RoutingTypes/MqttAddress";
import JoynrMessage from "../JoynrMessage";

import * as JSONSerializer from "../../util/JSONSerializer";
import LoggingManager from "../../system/LoggingManager";
import SharedMqttClient = require("./SharedMqttClient");

const log = LoggingManager.getLogger("joynr/messaging/mqtt/MqttMessagingStub");

class MqttMessagingStub {
    public static PRIORITY_LOW = "/low";
    private address: MqttAddress;
    private client: SharedMqttClient;

    /**
     * @param settings the settings object for this constructor call
     * @param settings.address the mqtt address of the message destination
     * @param settings.client the mqtt client to be used to transmit messages
     */
    public constructor(settings: { address: MqttAddress; client: SharedMqttClient }) {
        this.address = settings.address;
        this.client = settings.client;
    }

    /**
     * @param message the message to transmit
     */
    public transmit(message: JoynrMessage): Promise<void> {
        log.debug(`transmit message: "${JSONSerializer.stringify(message)}"`);
        let topic = this.address.topic;
        if (!(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST === message.type)) {
            topic += MqttMessagingStub.PRIORITY_LOW;
        }

        return this.client.send(topic, message);
    }
}

export = MqttMessagingStub;
