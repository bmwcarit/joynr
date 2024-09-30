/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration.context;

import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getBeansXml;
import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getExtension;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import io.joynr.jeeintegration.api.JoynrJeeMessageScoped;
import io.joynr.jeeintegration.context.RegisterJoynrJeeMessageContextExtension;
import jakarta.enterprise.context.ContextNotActiveException;
import jakarta.enterprise.context.spi.Context;
import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.enterprise.inject.spi.Extension;
import jakarta.inject.Inject;

import junit.framework.TestResult;
import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.spi.ArquillianProxyException;
import org.jboss.shrinkwrap.api.Archive;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.context.JoynrJeeMessageContext;

/**
 * Integration tests for the {@link JoynrJeeMessageContext}.
 */
@RunWith(Arquillian.class)
public class JoynrJeeMessageContextTest {
    private static final Logger logger = LoggerFactory.getLogger(JoynrJeeMessageContextTest.class);

    @Deployment
    public static Archive<?> getDeployment() {
        return ShrinkWrap.create(JavaArchive.class)
                         .addClasses(RegisterJoynrJeeMessageContextExtension.class,
                                     JoynrJeeMessageContext.class,
                                     JoynrJeeMessageScoped.class,
                                     TestResult.class,
                                     MessageScopedBean.class)
                         .addPackages(true, "org.slf4j")
                         .addAsServiceProvider(Extension.class, RegisterJoynrJeeMessageContextExtension.class)
                         .addAsManifestResource(getBeansXml())
                         .addAsManifestResource(getExtension());
    }

    @Inject
    private BeanManager beanManager;

    @Inject
    private MessageScopedBean messageScopedBean;

    @Test(timeout = 10000)
    public void testContextRegistered() {
        JoynrJeeMessageContext.getInstance().activate();

        Context result = beanManager.getContext(JoynrJeeMessageScoped.class);

        assertNotNull(result);
        assertTrue(result instanceof JoynrJeeMessageContext);

        JoynrJeeMessageContext.getInstance().deactivate();
        try {
            result = beanManager.getContext(JoynrJeeMessageScoped.class);
            fail("Shouldn't get it after deactivation.");
        } catch (ContextNotActiveException e) {
            logger.trace("Context not available after deactivation as expected.");
        }
    }

    @Test
    public void testMessageScopedBeanOutOfScope() {
        assertNotNull(messageScopedBean);
        try {
            messageScopedBean.ping("Hello world");
        } catch (final ArquillianProxyException expectedException) {
            assertNotNull(expectedException);
            assertNotNull(expectedException.getCause());
            assertTrue(expectedException.getCause() instanceof ContextNotActiveException);
        } catch (final ContextNotActiveException expectedException) {
            assertNotNull(expectedException);
        } catch (final Exception unexpectedException) {
            fail("Unexpected exception occurred. Class: " + unexpectedException.getClass().getSimpleName()
                    + "; message: " + unexpectedException.getMessage());
        }
    }

    @Test(timeout = 10000)
    public void testMessageScopedBean() {
        assertNotNull(messageScopedBean);
        JoynrJeeMessageContext.getInstance().activate();
        assertTrue(JoynrJeeMessageContext.getInstance().isActive());

        assertEquals("test", messageScopedBean.ping("test"));

        JoynrJeeMessageContext.getInstance().deactivate();
        assertFalse(JoynrJeeMessageContext.getInstance().isActive());
    }

}
