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
package itest.io.joynr.jeeintegration.context;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.File;

import javax.enterprise.context.ContextNotActiveException;
import javax.enterprise.context.spi.Context;
import javax.enterprise.inject.spi.BeanManager;
import com.google.inject.Inject;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.spi.TestResult;
import org.jboss.shrinkwrap.api.Archive;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.JoynrJeeMessageScoped;
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
                         .addClasses(JoynrJeeMessageScoped.class,
                                     JoynrJeeMessageContext.class,
                                     TestResult.class,
                                     MessageScopedBean.class)
                         .addAsManifestResource(new File("src/main/resources/META-INF/beans.xml"))
                         .addAsManifestResource(new File("src/main/resources/META-INF/services/javax.enterprise.inject.spi.Extension"));
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

    @Test(expected = ContextNotActiveException.class)
    public void testMessageScopedBeanOutOfScope() {
        assertNotNull(messageScopedBean);
        messageScopedBean.ping("Hello world");
    }

    @Test(timeout = 10000)
    public void testMessageScopedBean() {
        assertNotNull(messageScopedBean);
        JoynrJeeMessageContext.getInstance().activate();
        assertEquals("test", messageScopedBean.ping("test"));
        JoynrJeeMessageContext.getInstance().deactivate();
    }

}
