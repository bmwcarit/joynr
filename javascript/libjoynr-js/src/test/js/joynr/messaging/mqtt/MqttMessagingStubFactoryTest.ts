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

import MqttAddress from "../../../../../main/js/generated/joynr/system/RoutingTypes/MqttAddress";
import MqttMessagingStubFactory from "../../../../../main/js/joynr/messaging/mqtt/MqttMessagingStubFactory";

describe("libjoynr-js.joynr.messaging.mqtt.MqttMessagingStubFactory", () => {
    let mqttMessagingStubFactory: MqttMessagingStubFactory, mqttClient: any;
    let mqttAddress: MqttAddress, brokerUri: any, topic: any, joyrnMessageMock: any;

    beforeEach(done => {
        mqttClient = {
            send: jest.fn().mockResolvedValue(undefined)
        };

        mqttMessagingStubFactory = new MqttMessagingStubFactory({
            client: mqttClient
        });

        brokerUri = "testBrokerUri";
        topic = "testTopic";
        mqttAddress = new MqttAddress({
            brokerUri,
            topic
        });
        joyrnMessageMock = {};

        done();
    });

    it("is instantiable and of correct type", () => {
        expect(MqttMessagingStubFactory).toBeDefined();
        expect(typeof MqttMessagingStubFactory).toEqual("function");
        expect(mqttMessagingStubFactory).toBeDefined();
        expect(mqttMessagingStubFactory.build).toBeDefined();
        expect(typeof mqttMessagingStubFactory.build).toEqual("function");
    });

    it("creates a messaging stub and uses it correctly", async () => {
        const mqttMessagingStub = mqttMessagingStubFactory.build(mqttAddress);
        await mqttMessagingStub.transmit(joyrnMessageMock);
        expect(mqttClient.send).toHaveBeenCalledWith(expect.any(String), joyrnMessageMock);
    });
});
