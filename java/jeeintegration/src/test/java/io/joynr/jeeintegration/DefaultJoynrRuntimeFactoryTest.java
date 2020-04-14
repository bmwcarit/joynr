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
package io.joynr.jeeintegration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Type;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.ScheduledExecutorService;

import javax.ejb.Stateless;
import javax.enterprise.inject.Instance;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.mockito.Mockito;

import com.google.inject.Binding;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.TypeLiteral;
import com.google.inject.name.Names;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.JoynrMessageProcessor;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.NoOpRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;
import io.joynr.messaging.mqtt.MqttClientIdProvider;
import io.joynr.statusmetrics.JoynrStatusMetrics;
import io.joynr.statusmetrics.MqttStatusReceiver;
import io.joynr.messaging.persistence.MessagePersister;
import io.joynr.messaging.persistence.NoOpMessagePersister;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.JoynrRuntime;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.Request;
import joynr.jeeintegration.servicelocator.MyService;
import joynr.jeeintegration.servicelocator.MyServiceProvider;
import joynr.jeeintegration.servicelocator.MyServiceSync;

/**
 * Unit tests for the {@link DefaultJoynrRuntimeFactory}.
 */
public class DefaultJoynrRuntimeFactoryTest {

    private static final String LOCAL_DOMAIN = "local-domain";
    private static final String CHANNEL_ID = "channel_id";

    private ScheduledExecutorService scheduledExecutorService;

    private DefaultJoynrRuntimeFactory fixture;
    @Rule
    public ExpectedException expectedException = ExpectedException.none();

    @Stateless
    private class JoynrMessageProcessorTest implements JoynrMessageProcessor {
        @Override
        public MutableMessage processOutgoing(MutableMessage joynrMessage) {
            joynrMessage.getCustomHeaders().put("test", "test");
            return joynrMessage;
        }

        @Override
        public ImmutableMessage processIncoming(ImmutableMessage joynrMessage) {
            return joynrMessage;
        }
    }

    private void createFixture() throws Exception {
        createFixture(null);
    }

    private void createFixture(Properties additionalProperties) throws Exception {
        createFixture(createPropertiesMock(additionalProperties), createLocalDomainMock());
    }

    @SuppressWarnings("unchecked")
    private Instance<Properties> createPropertiesMock(Properties additionalProperties) {
        Instance<Properties> joynrProperties = mock(Instance.class);
        Properties joynrPropertiesValues = new Properties();
        joynrPropertiesValues.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_CONTEXT_ROOT, "/");
        joynrPropertiesValues.setProperty(MessagingPropertyKeys.PROPERTY_SERVLET_HOST_PATH, "http://localhost:8080");
        joynrPropertiesValues.setProperty(MessagingPropertyKeys.CHANNELID, CHANNEL_ID);
        if (additionalProperties != null) {
            joynrPropertiesValues.putAll(additionalProperties);
        }

        when(joynrProperties.isAmbiguous()).thenReturn(false);
        when(joynrProperties.isUnsatisfied()).thenReturn(false);
        when(joynrProperties.get()).thenReturn(joynrPropertiesValues);

        return joynrProperties;
    }

    @SuppressWarnings("unchecked")
    private Instance<String> createLocalDomainMock() {
        Instance<String> joynrLocalDomain = mock(Instance.class);

        when(joynrLocalDomain.get()).thenReturn(LOCAL_DOMAIN);
        when(joynrLocalDomain.isAmbiguous()).thenReturn(false);
        when(joynrLocalDomain.isUnsatisfied()).thenReturn(false);

        return joynrLocalDomain;
    }

    @SuppressWarnings("unchecked")
    private void createFixture(Instance<Properties> joynrProperties,
                               Instance<String> joynrLocalDomain) throws Exception {
        Instance<RawMessagingPreprocessor> rawMessageProcessor = mock(Instance.class);
        when(rawMessageProcessor.get()).thenReturn(new NoOpRawMessagingPreprocessor());
        Instance<MessagePersister> messagePersisterInstance = mock(Instance.class);
        when(messagePersisterInstance.get()).thenReturn(new NoOpMessagePersister());
        BeanManager beanManager = mock(BeanManager.class);
        Bean<JoynrMessageProcessor> bean = mock(Bean.class);
        when(bean.create(Mockito.any())).thenReturn(new JoynrMessageProcessorTest());
        when(beanManager.getBeans(Mockito.<Type> eq(JoynrMessageProcessor.class),
                                  Mockito.<Annotation> any())).thenReturn(new HashSet<Bean<?>>(Arrays.asList(bean)));

        final String mqttClientId = "someTestMqttClientId";
        MqttClientIdProvider mqttClientIdProvider = mock(MqttClientIdProvider.class);
        when(mqttClientIdProvider.getClientId()).thenReturn(mqttClientId);
        Instance<MqttClientIdProvider> mqttClientIdProviderInstance = mock(Instance.class);
        when(mqttClientIdProviderInstance.get()).thenReturn(mqttClientIdProvider);

        fixture = new DefaultJoynrRuntimeFactory(joynrProperties,
                                                 joynrLocalDomain,
                                                 rawMessageProcessor,
                                                 mqttClientIdProviderInstance,
                                                 messagePersisterInstance,
                                                 beanManager,
                                                 mock(MqttStatusReceiver.class),
                                                 mock(JoynrStatusMetrics.class));
        scheduledExecutorService = mock(ScheduledExecutorService.class);
        Field executorField = DefaultJoynrRuntimeFactory.class.getDeclaredField("scheduledExecutorService");
        executorField.setAccessible(true);
        executorField.set(fixture, scheduledExecutorService);
    }

    @Test
    public void testGetLocalDomain() throws Exception {
        createFixture();
        String result = fixture.getLocalDomain();
        assertNotNull(result);
        assertEquals(LOCAL_DOMAIN, result);
    }

    @Test
    public void testJoynrMessageProcessorAdded() throws Exception {
        createFixture();
        Injector injector = fixture.getInjector();
        List<Binding<JoynrMessageProcessor>> bindings = injector.findBindingsByType(new TypeLiteral<JoynrMessageProcessor>() {
        });
        assertEquals(1, bindings.size());
    }

    @Test
    public void testJoynrMessageProcessorUsed() throws Exception {
        createFixture();
        Injector injector = fixture.getInjector();
        MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
        MutableMessage request = messageFactory.createRequest("from",
                                                              "to",
                                                              new Request("name", new Object[0], new Class[0]),
                                                              new MessagingQos());
        assertEquals("test", request.getCustomHeaders().get("test"));
    }

    @Test
    public void testClusterableParticipantIdsAdded() throws Exception {
        createFixture();
        JoynrRuntime joynrRuntime = fixture.create(new HashSet<Class<?>>(Arrays.asList(MyServiceSync.class)));
        assertNotNull(joynrRuntime);
        Properties properties = fixture.getInjector()
                                       .getInstance(Key.get(Properties.class,
                                                            Names.named(MessagingPropertyKeys.JOYNR_PROPERTIES)));
        assertNotNull(properties);
        String key = (ParticipantIdKeyUtil.JOYNR_PARTICIPANT_PREFIX + LOCAL_DOMAIN + "." + MyService.INTERFACE_NAME
                + ".v" + ProviderAnnotations.getMajorVersion(MyServiceProvider.class)).toLowerCase().replace("/", ".");
        assertTrue(properties.containsKey(key));
        String value = properties.getProperty(key);
        assertNotNull(value);
        assertEquals((LOCAL_DOMAIN + "." + CHANNEL_ID + "." + MyService.INTERFACE_NAME + ".v"
                + ProviderAnnotations.getMajorVersion(MyServiceProvider.class)).replace("/", "."), value);
    }

    @Test
    public void testNoOverrideForManuallyAddedParticipantIds() throws Exception {
        Properties joynrProperties = new Properties();
        String key = (ParticipantIdKeyUtil.JOYNR_PARTICIPANT_PREFIX + LOCAL_DOMAIN + "."
                + MyService.INTERFACE_NAME).toLowerCase().replace("/", ".");
        joynrProperties.setProperty(key, "myvalue");
        createFixture(joynrProperties);

        JoynrRuntime joynrRuntime = fixture.create(new HashSet<Class<?>>(Arrays.asList(MyServiceSync.class)));
        assertNotNull(joynrRuntime);
        Properties properties = fixture.getInjector()
                                       .getInstance(Key.get(Properties.class,
                                                            Names.named(MessagingPropertyKeys.JOYNR_PROPERTIES)));
        assertNotNull(properties);
        assertTrue(properties.containsKey(key));
        String value = properties.getProperty(key);
        assertNotNull(value);
        assertEquals("myvalue", value);
    }

    @Test
    public void testJoynrLocalDomainUnsatisfiedThrows() throws Exception {
        Instance<String> joynrLocalDomain = createLocalDomainMock();
        when(joynrLocalDomain.isUnsatisfied()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage("No local domain name specified. Please provide a value for the local domain via @JoynrLocalDomain in your configuration EJB.");
        createFixture(createPropertiesMock(null), joynrLocalDomain);
    }

    @Test
    public void testJoynrLocalDomainAmbiguousThrows() throws Exception {
        Instance<String> joynrLocalDomain = createLocalDomainMock();
        when(joynrLocalDomain.isAmbiguous()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage("Multiple local domain names specified. Please provide only one configuration EJB containing a value for the local domain via @JoynrLocalDomain.");
        createFixture(createPropertiesMock(null), joynrLocalDomain);
    }

    @Test
    public void testJoynrPropertiesUnsatisfiedDoesNotThrow() throws Exception {
        Instance<Properties> joynrProperties = createPropertiesMock(null);
        when(joynrProperties.isUnsatisfied()).thenReturn(true);

        createFixture(joynrProperties, createLocalDomainMock());
    }

    @Test
    public void testJoynrPropertiesAmbiguousThrows() throws Exception {
        Instance<Properties> joynrProperties = createPropertiesMock(null);
        when(joynrProperties.isAmbiguous()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage("Multiple joynrProperties specified. Please provide only one configuration EJB containing a value for the joynrProperties via @JoynrProperties.");
        createFixture(joynrProperties, createLocalDomainMock());
    }
}
