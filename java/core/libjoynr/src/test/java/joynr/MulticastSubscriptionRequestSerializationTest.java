/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package joynr;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.pubsub.SubscriptionQos;
import io.joynr.util.ObjectMapper;

public class MulticastSubscriptionRequestSerializationTest {

    private static final Logger logger = LoggerFactory.getLogger(MulticastSubscriptionRequestSerializationTest.class);

    @Test
    public void testSerializeAndDeserialize() throws Exception {
        ObjectMapper objectMapper = new ObjectMapper();
        String multicastId = "multicastId";
        String multicastName = "multicastName";
        String subscriptionId = "subscriptionId";
        SubscriptionQos qos = new OnChangeSubscriptionQos();
        MulticastSubscriptionRequest multicastSubscriptionRequest = new MulticastSubscriptionRequest(multicastId,
                                                                                                     subscriptionId,
                                                                                                     multicastName,
                                                                                                     qos);

        String serializedValue = objectMapper.writeValueAsString(multicastSubscriptionRequest);
        logger.trace("Serialized {} is {}", multicastSubscriptionRequest, serializedValue);
        assertNotNull(serializedValue);

        MulticastSubscriptionRequest deserializedMulticastSubscriptionRequest = objectMapper.readValue(serializedValue,
                                                                                                       MulticastSubscriptionRequest.class);
        assertNotNull(deserializedMulticastSubscriptionRequest);

        assertEquals(multicastId, deserializedMulticastSubscriptionRequest.getMulticastId());
        assertEquals(multicastName, deserializedMulticastSubscriptionRequest.getSubscribedToName());
        assertEquals(subscriptionId, deserializedMulticastSubscriptionRequest.getSubscriptionId());
        assertEquals(qos, deserializedMulticastSubscriptionRequest.getQos());
    }

}
