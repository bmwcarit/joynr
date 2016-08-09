/**
 *
 */
package test.io.joynr.jeeintegration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Type;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.ScheduledExecutorService;

import javax.ejb.Stateless;
import javax.enterprise.inject.Instance;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import com.google.common.collect.Sets;
import com.google.inject.Binding;
import com.google.inject.Injector;
import com.google.inject.TypeLiteral;
import io.joynr.dispatching.JoynrMessageFactory;
import io.joynr.dispatching.JoynrMessageProcessor;
import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import joynr.JoynrMessage;
import joynr.Request;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mockito;

/**
 * Unit tests for the {@link DefaultJoynrRuntimeFactory}.
 */
public class DefaultJoynrRuntimeFactoryTest {

    private static final String LOCAL_DOMAIN = "local-domain";

    private ScheduledExecutorService scheduledExecutorService;

    private DefaultJoynrRuntimeFactory fixture;

    @Stateless
    private class JoynrMessageProcessorTest implements JoynrMessageProcessor {
        @Override
        public JoynrMessage process(JoynrMessage joynrMessage) {
            joynrMessage.getHeader().put("test", "test");
            return joynrMessage;
        }
    }

    @SuppressWarnings("unchecked")
    @Before
    public void setup() throws Exception {
        Instance<Properties> joynrProperties = mock(Instance.class);
        Properties joynrPropertiesValues = new Properties();
        joynrPropertiesValues.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT, "/");
        joynrPropertiesValues.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://localhost:8080");
        joynrPropertiesValues.setProperty("joynr.jeeintegration.broker.uri", "http://localhost:18080");
        when(joynrProperties.get()).thenReturn(joynrPropertiesValues);
        Instance<String> joynrLocalDomain = mock(Instance.class);
        when(joynrLocalDomain.get()).thenReturn(LOCAL_DOMAIN);
        BeanManager beanManager = mock(BeanManager.class);
        Bean<JoynrMessageProcessor> bean = mock(Bean.class);
        when(bean.create(Mockito.any())).thenReturn(new JoynrMessageProcessorTest());
        when(beanManager.getBeans(Mockito.<Type> eq(JoynrMessageProcessor.class), Mockito.<Annotation> any())).thenReturn(Sets.newHashSet(bean));
        fixture = new DefaultJoynrRuntimeFactory(joynrProperties, joynrLocalDomain, beanManager);
        scheduledExecutorService = mock(ScheduledExecutorService.class);
        Field executorField = DefaultJoynrRuntimeFactory.class.getDeclaredField("scheduledExecutorService");
        executorField.setAccessible(true);
        executorField.set(fixture, scheduledExecutorService);
    }

    @Test
    public void testGetLocalDomain() {
        String result = fixture.getLocalDomain();
        assertNotNull(result);
        assertEquals(LOCAL_DOMAIN, result);
    }

    @Test
    public void testJoynrMessageProcessorAdded() {
        Injector injector = fixture.getInjector();
        List<Binding<JoynrMessageProcessor>> bindings = injector.findBindingsByType(new TypeLiteral<JoynrMessageProcessor>() {
        });
        assertEquals(1, bindings.size());
    }

    @Test
    public void testJoynrMessageProcessUsed() {
        Injector injector = fixture.getInjector();
        JoynrMessageFactory joynrMessageFactory = injector.getInstance(JoynrMessageFactory.class);
        JoynrMessage request = joynrMessageFactory.createRequest("from",
                                                                 "to",
                                                                 new Request("name", new Object[0], new Class[0]),
                                                                 new MessagingQos());
        assertEquals("test", request.getHeader().get("test"));
    }

}
