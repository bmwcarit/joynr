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
package io.joynr.jeeintegration.context;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import jakarta.enterprise.context.spi.Contextual;
import jakarta.enterprise.context.spi.CreationalContext;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

/**
 * Unit tests for {@link JoynrJeeMessageContext}.
 */
public class JoynrJeeMessageContextTest {

    @Before
    public void setup() {
        assertFalse(JoynrJeeMessageContext.getInstance().isActive());
        JoynrJeeMessageContext.getInstance().activate();
        assertTrue(JoynrJeeMessageContext.getInstance().isActive());
    }

    @After
    public void cleanup() {
        JoynrJeeMessageContext.getInstance().deactivate();
    }

    @Test
    public void testNotActiveAfterDeactivation() {
        assertTrue(JoynrJeeMessageContext.getInstance().isActive());
        JoynrJeeMessageContext.getInstance().deactivate();
        assertFalse(JoynrJeeMessageContext.getInstance().isActive());
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testObtainNullForNonCreated() {
        assertNull(JoynrJeeMessageContext.getInstance().get(mock(Contextual.class)));
    }

    @SuppressWarnings({ "rawtypes", "unchecked" })
    @Test
    public void testCreateNewInstanceAndSubsequentRetrievalOfExistingObject() {
        CreationalContext creationalContext = mock(CreationalContext.class);
        Contextual contextual = mock(Contextual.class);
        when(contextual.create(creationalContext)).thenReturn("test");

        Object result = JoynrJeeMessageContext.getInstance().get(contextual, creationalContext);

        assertNotNull(result);
        assertEquals("test", result);
        verify(contextual).create(creationalContext);

        assertEquals("test", JoynrJeeMessageContext.getInstance().get(contextual));

        reset(contextual);
        reset(creationalContext);

        assertEquals("test", JoynrJeeMessageContext.getInstance().get(contextual, creationalContext));
        verify(contextual, never()).create(creationalContext);
    }
}
