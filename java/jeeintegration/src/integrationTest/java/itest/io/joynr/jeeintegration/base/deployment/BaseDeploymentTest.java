/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration.base.deployment;

import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.inject.Inject;
import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.shrinkwrap.api.Archive;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Test;
import org.junit.runner.RunWith;

import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getBeansXml;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

@RunWith(Arquillian.class)
public class BaseDeploymentTest {
    @Deployment
    public static Archive<?> getDeployment() {
        return ShrinkWrap.create(JavaArchive.class).addClass(TestComponent.class).addAsManifestResource(getBeansXml());
    }

    @Inject
    private TestComponent testComponent;
    @Inject
    private BeanManager beanManager;

    @Test
    public void testBaseDeployment() {
        assertNotNull(testComponent);
        assertNotNull(beanManager);
        final String defaultPrefix = testComponent.getMessagePrefix();
        assertNotNull(defaultPrefix);
        assertEquals("Message: ", defaultPrefix);
        final String firstMessage = testComponent.message("Have a nice day!");
        assertNotNull(firstMessage);
        assertEquals("Message: Have a nice day!", firstMessage);
        testComponent.setMessagePrefix("ALERT! ");
        final String modifiedPrefix = testComponent.getMessagePrefix();
        assertNotNull(modifiedPrefix);
        assertEquals("ALERT! ", modifiedPrefix);
        final String secondMessage = testComponent.message("Thunderstorm incoming!");
        assertNotNull(secondMessage);
        assertEquals("ALERT! Thunderstorm incoming!", secondMessage);
    }
}
