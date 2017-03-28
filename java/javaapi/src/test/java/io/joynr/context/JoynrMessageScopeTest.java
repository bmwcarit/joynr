package io.joynr.context;

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
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.ProvisionException;

/**
 * Unit tests for the {@link JoynrMessageScope}.
 */
public class JoynrMessageScopeTest {

    private static final Logger logger = LoggerFactory.getLogger(JoynrMessageScopeTest.class);

    @JoynrMessageScoped
    private static class ScopedInstance {
        private String value;

        public String getValue() {
            return value;
        }

        public void setValue(String value) {
            this.value = value;
        }
    }

    @Test
    public void testGetScopedInstance() {
        Injector injector = setupInjectorWithScope();
        JoynrMessageScope scope = injector.getInstance(JoynrMessageScope.class);
        scope.activate();
        try {
            ScopedInstance instance = injector.getInstance(ScopedInstance.class);
            logger.debug("On first getInstance got: {}", instance);
            assertNotNull(instance);
            assertNull(instance.getValue());
            instance.setValue("myValue");
            ScopedInstance secondInstance = injector.getInstance(ScopedInstance.class);
            logger.debug("On second getInstance got: {}", secondInstance);
            assertNotNull(secondInstance);
            assertEquals("myValue", secondInstance.getValue());
        } finally {
            scope.deactivate();
        }
        try {
            injector.getInstance(ScopedInstance.class);
            fail("Should not be able to get scoped bean from inactive scope.");
        } catch (ProvisionException e) {
            // expected
            assertTrue(e.getCause() instanceof IllegalStateException);
        }
    }

    private Injector setupInjectorWithScope(Module... modules) {
        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                install(new JoynrMessageScopeModule());
            }
        });
        return injector;
    }

}
