package io.joynr.capabilities;

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

import io.joynr.ProvidedBy;
import io.joynr.provider.JoynrInterface;
import org.junit.Test;

public class ParticipantIdKeyUtilTest {

    @JoynrInterface(name = "interfaceName", provides = SyncInterface.class)
    private static interface AnnotatedInterface {
    }

    @ProvidedBy(AnnotatedInterface.class)
    private static interface SyncInterface {
    }

    private String domain = "domain";
    private String interfaceName = "interfaceName";

    @Test
    public void testGenerateKeyFromStrings() {
        String key = ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, interfaceName);
        assertKeyCorrect(key);
    }

    @Test
    public void testGenerateKeyFromAnnotatedInterface() {
        String key = ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, AnnotatedInterface.class);
        assertKeyCorrect(key);
    }

    @Test
    public void testGenerateKeyFromSyncInterface() {
        String key = ParticipantIdKeyUtil.getProviderParticipantIdKey(domain, SyncInterface.class);
        assertKeyCorrect(key);
    }

    private void assertKeyCorrect(String key) {
        assertNotNull(key);
        assertEquals((ParticipantIdKeyUtil.JOYNR_PARTICIPANT_PREFIX + domain + "." + interfaceName).toLowerCase(), key);
    }
}
