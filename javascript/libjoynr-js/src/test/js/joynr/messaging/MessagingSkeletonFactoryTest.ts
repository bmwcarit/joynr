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

import MessagingSkeletonFactory from "joynr/joynr/messaging/MessagingSkeletonFactory";

import MqttAddress from "joynr/generated/joynr/system/RoutingTypes/MqttAddress";

describe("libjoynr-js.joynr.messaging.MessagingSkeletonFactory", () => {
    let multicastSkeletons: MessagingSkeletonFactory;
    let mqttMessagingSkeleton: any;

    beforeEach(() => {
        mqttMessagingSkeleton = {
            shutdown: jest.fn()
        };
        multicastSkeletons = new MessagingSkeletonFactory();
        const messagingSkeletons: Record<string, any> = {};
        messagingSkeletons[MqttAddress._typeName] = mqttMessagingSkeleton;
        multicastSkeletons.setSkeletons(messagingSkeletons);
    });

    it("provides expected API", () => {
        expect(MessagingSkeletonFactory).toBeDefined();
        expect(multicastSkeletons).toBeDefined();
        expect(multicastSkeletons).toBeInstanceOf(MessagingSkeletonFactory);
        expect(multicastSkeletons.getSkeleton).toBeDefined();
    });

    it("returns the appropriate messaging skeleton depending on object type", () => {
        expect(multicastSkeletons.getSkeleton(new MqttAddress(undefined as any))).toBe(mqttMessagingSkeleton);
    });
});
