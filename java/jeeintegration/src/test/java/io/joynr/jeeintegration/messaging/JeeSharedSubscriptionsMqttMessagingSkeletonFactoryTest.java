/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.jeeintegration.messaging;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.messaging.IMessagingSkeleton;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessageInProgressObserver;
import io.joynr.messaging.mqtt.MqttMessagingSkeleton;
import io.joynr.messaging.mqtt.SharedSubscriptionsMqttMessagingSkeleton;
import io.joynr.messaging.routing.MessageProcessedHandler;
import joynr.system.RoutingTypes.MqttAddress;

@RunWith(MockitoJUnitRunner.class)
public class JeeSharedSubscriptionsMqttMessagingSkeletonFactoryTest {

    private static final String[] GBIDS = new String[]{ "gbid1", "gbid2" };

    @Mock
    private MqttClientFactory mockMqttClientFactory;

    @Mock
    private MessageProcessedHandler mockMessageProcessedHandler;

    @Mock
    protected MqttMessageInProgressObserver mqttMessageInProgressObserver;

    @Test
    public void createsExpectedSkeletons() {
        JeeSharedSubscriptionsMqttMessagingSkeletonFactory factory = new JeeSharedSubscriptionsMqttMessagingSkeletonFactory(GBIDS,
                                                                                                                            new MqttAddress(),
                                                                                                                            42,
                                                                                                                            new MqttAddress(),
                                                                                                                            null,
                                                                                                                            mockMessageProcessedHandler,
                                                                                                                            mockMqttClientFactory,
                                                                                                                            "channelId",
                                                                                                                            null,
                                                                                                                            null,
                                                                                                                            null,
                                                                                                                            null,
                                                                                                                            null,
                                                                                                                            false,
                                                                                                                            "",
                                                                                                                            mqttMessageInProgressObserver);
        for (String gbid : GBIDS) {
            IMessagingSkeleton skeleton = factory.getSkeleton(new MqttAddress(gbid, ""));
            assertTrue(JeeSharedSubscriptionsMqttMessagingSkeleton.class.isInstance(skeleton));
            assertEquals(JeeSharedSubscriptionsMqttMessagingSkeleton.class, skeleton.getClass());
            assertNotEquals(SharedSubscriptionsMqttMessagingSkeleton.class, skeleton.getClass());
            assertNotEquals(MqttMessagingSkeleton.class, skeleton.getClass());
        }
    }

}
