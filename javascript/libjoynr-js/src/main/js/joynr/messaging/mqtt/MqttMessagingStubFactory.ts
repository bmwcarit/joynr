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

import MqttMessagingStub from "./MqttMessagingStub";
import SharedMqttClient = require("./SharedMqttClient");

class MqttMessagingStubFactory {
    private readonly client: SharedMqttClient;
    /**
     * @constructor
     * @param settings
     * @param settings.client the mqtt client
     */
    public constructor(settings: { client: SharedMqttClient }) {
        this.client = settings.client;
    }

    public build(address: MqttAddress): MqttMessagingStub {
        return new MqttMessagingStub({
            address,
            client: this.client
        });
    }
}

export = MqttMessagingStubFactory;
