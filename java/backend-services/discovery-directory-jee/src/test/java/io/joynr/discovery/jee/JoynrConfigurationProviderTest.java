package io.joynr.discovery.jee;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.util.Properties;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import org.junit.Test;

public class JoynrConfigurationProviderTest {

    @Test
    public void testCapabilitiesDirectoryParticipantIdReadCorrectly() {
        JoynrConfigurationProvider joynrConfigurationProvider = new JoynrConfigurationProvider();
        Properties joynrProperties = joynrConfigurationProvider.getJoynrProperties();
        assertNotNull(joynrProperties);
        String key = ParticipantIdKeyUtil.getProviderParticipantIdKey(joynrConfigurationProvider.getJoynrLocalDomain(),
                                                                      GlobalCapabilitiesDirectoryProvider.class);
        assertTrue(joynrProperties.containsKey(key));
        assertEquals("capabilitiesdirectory_participantid", joynrProperties.getProperty(key));
    }
}
