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
package io.joynr.integration;

import org.junit.Before;

import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.capabilities.CapabilityUtils;
import io.joynr.messaging.MessagingPropertyKeys;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.types.GlobalDiscoveryEntry;

/**
 * Test that the correct backend connection is used for global Discovery (add, lookup, remove).
 */
public abstract class MqttMultipleBackendDiscoveryAbstractTest extends AbstractMqttMultipleBackendTest {

    @Before
    public void setUp() throws InterruptedException {
        super.setUp();
        createJoynrRuntime();
    }

    protected String getGcdTopic() {
        GlobalDiscoveryEntry gcdDiscoveryEntry = injector.getInstance(Key.get(GlobalDiscoveryEntry.class,
                                                                              Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY)));
        String gcdTopic = ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(gcdDiscoveryEntry)).getTopic();
        gcdTopic += "/low";
        return gcdTopic;
    }

    protected String getGcdParticipantId() {
        GlobalDiscoveryEntry gcdDiscoveryEntry = injector.getInstance(Key.get(GlobalDiscoveryEntry.class,
                                                                              Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY)));
        return gcdDiscoveryEntry.getParticipantId();
    }

}
