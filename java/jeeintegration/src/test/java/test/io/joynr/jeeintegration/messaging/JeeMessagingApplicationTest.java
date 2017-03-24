/**
 *
 */
package test.io.joynr.jeeintegration.messaging;

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

import java.util.Set;

import org.junit.Test;

import io.joynr.jeeintegration.messaging.JeeMessagingApplication;
import io.joynr.jeeintegration.messaging.JeeMessagingEndpoint;

/**
 * Unit tests for the {@link JeeMessagingApplication}.
 */
public class JeeMessagingApplicationTest {

    @Test
    public void testGetClasses() {
        JeeMessagingApplication subject = new JeeMessagingApplication();
        Set<Class<?>> result = subject.getClasses();
        assertNotNull(result);
        assertEquals(1, result.size());
        assertEquals(JeeMessagingEndpoint.class, result.iterator().next());
    }

}
