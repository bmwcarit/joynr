/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2023 BMW Car IT GmbH
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
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.ClusterControllerRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import joynr.ImmutableMessage;
import joynr.MutableMessage;
import joynr.Request;
import joynr.jeeintegration.servicelocator.MyService;
import joynr.jeeintegration.servicelocator.MyServiceProvider;
import joynr.jeeintegration.servicelocator.MyServiceSync;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.mockito.Mockito;

import jakarta.ejb.Stateless;
import jakarta.enterprise.inject.Instance;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;
import java.lang.annotation.Annotation;
import java.lang.reflect.Field;
import java.lang.reflect.Type;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.ScheduledExecutorService;

import static io.joynr.jeeintegration.DefaultJoynrRuntimeFactory.ERROR_LOCAL_DOMAIN_IS_EMPTY;
import static io.joynr.jeeintegration.DefaultJoynrRuntimeFactory.ERROR_MULTIPLE_ID_CLIENTS;
import static io.joynr.jeeintegration.DefaultJoynrRuntimeFactory.ERROR_MULTIPLE_LOCAL_DOMAINS;
import static io.joynr.jeeintegration.DefaultJoynrRuntimeFactory.ERROR_MULTIPLE_PREPROCESSORS;
import static io.joynr.jeeintegration.DefaultJoynrRuntimeFactory.ERROR_MULTIPLE_PROPERTIES;
import static io.joynr.jeeintegration.DefaultJoynrRuntimeFactory.ERROR_NO_LOCAL_DOMAIN;
import static io.joynr.messaging.mqtt.MqttModule.PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

/**
 * Unit tests for the {@link DefaultJoynrRuntimeFactory}.
 */
public class DefaultJoynrRuntimeFactoryTest {

    private static final String LOCAL_DOMAIN = "local-domain";
    private static final String CHANNEL_ID = "channel_id";

    private DefaultJoynrRuntimeFactory fixture;

    @SuppressWarnings("deprecation")
    @Rule
    public ExpectedException expectedException = ExpectedException.none();

    @Stateless
    private static class JoynrMessageProcessorTest implements JoynrMessageProcessor {
        @Override
        public MutableMessage processOutgoing(final MutableMessage joynrMessage) {
            HashMap<String, String> hashMap = new HashMap<>();
            hashMap.put("test", "test");
            joynrMessage.setCustomHeaders(hashMap);
            return joynrMessage;
        }

        @Override
        public ImmutableMessage processIncoming(final ImmutableMessage joynrMessage) {
            return joynrMessage;
        }
    }

    private void createFixture() throws Exception {
        createFixture(null);
    }

    private void createFixture(final Properties additionalProperties) throws Exception {
        createFixture(createPropertiesMock(additionalProperties), createLocalDomainMock());
    }

    @SuppressWarnings("unchecked")
    private Instance<Properties> createPropertiesMock(final Properties additionalProperties) {
        final Instance<Properties> joynrProperties = mock(Instance.class);
        final Properties joynrPropertiesValues = new Properties();
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
        final Instance<String> joynrLocalDomain = mock(Instance.class);

        when(joynrLocalDomain.get()).thenReturn(LOCAL_DOMAIN);
        when(joynrLocalDomain.isAmbiguous()).thenReturn(false);
        when(joynrLocalDomain.isUnsatisfied()).thenReturn(false);

        return joynrLocalDomain;
    }

    @SuppressWarnings("unchecked")
    private Instance<RawMessagingPreprocessor> createPreProcessorMock() {
        final Instance<RawMessagingPreprocessor> rawMessageProcessor = mock(Instance.class);
        when(rawMessageProcessor.get()).thenReturn(new NoOpRawMessagingPreprocessor());
        return rawMessageProcessor;
    }

    @SuppressWarnings("unchecked")
    private Instance<MqttClientIdProvider> createMqttClientIdProviderMock() {
        final String mqttClientId = "someTestMqttClientId";
        final MqttClientIdProvider mqttClientIdProvider = mock(MqttClientIdProvider.class);
        when(mqttClientIdProvider.getClientId()).thenReturn(mqttClientId);
        final Instance<MqttClientIdProvider> mqttClientIdProviderInstance = mock(Instance.class);
        when(mqttClientIdProviderInstance.get()).thenReturn(mqttClientIdProvider);
        return mqttClientIdProviderInstance;
    }

    private void createFixture(final Instance<Properties> joynrProperties,
                               final Instance<String> joynrLocalDomain) throws Exception {
        createFixture(joynrProperties, joynrLocalDomain, createPreProcessorMock(), createMqttClientIdProviderMock());
    }

    private void createFixtureWithPreProcessor(final Instance<RawMessagingPreprocessor> rawMessagePreProcessor) throws Exception {
        createFixture(createPropertiesMock(null),
                      createLocalDomainMock(),
                      rawMessagePreProcessor,
                      createMqttClientIdProviderMock());
    }

    private void createFixtureWithMqttClientIdProvider(final Instance<MqttClientIdProvider> mqttClientIdProvider) throws Exception {
        createFixture(createPropertiesMock(null),
                      createLocalDomainMock(),
                      createPreProcessorMock(),
                      mqttClientIdProvider);
    }

    @SuppressWarnings("unchecked")
    private void createFixture(final Instance<Properties> joynrProperties,
                               final Instance<String> joynrLocalDomain,
                               final Instance<RawMessagingPreprocessor> rawMessageProcessor,
                               final Instance<MqttClientIdProvider> mqttClientIdProvider) throws Exception {
        final BeanManager beanManager = mock(BeanManager.class);
        final Bean<JoynrMessageProcessor> bean = mock(Bean.class);
        when(bean.create(Mockito.any())).thenReturn(new JoynrMessageProcessorTest());
        when(beanManager.getBeans(Mockito.<Type> eq(JoynrMessageProcessor.class),
                                  Mockito.<Annotation> any())).thenReturn(new HashSet<>(List.of(bean)));

        final JoynrStatusMetricsReceiver joynrStatusMetrics = mock(JoynrStatusMetricsReceiver.class);

        fixture = new DefaultJoynrRuntimeFactory(joynrProperties,
                                                 joynrLocalDomain,
                                                 rawMessageProcessor,
                                                 mqttClientIdProvider,
                                                 beanManager,
                                                 joynrStatusMetrics);
        final ScheduledExecutorService scheduledExecutorService = mock(ScheduledExecutorService.class);
        final Field executorField = DefaultJoynrRuntimeFactory.class.getDeclaredField("scheduledExecutorService");
        executorField.setAccessible(true);
        executorField.set(fixture, scheduledExecutorService);
    }

    @Test
    public void testGetLocalDomain() throws Exception {
        createFixture();
        final String result = fixture.getLocalDomain();
        assertNotNull(result);
        assertEquals(LOCAL_DOMAIN, result);
    }

    @Test
    public void testGetRawMessagePreProcessor() throws Exception {
        createFixture();
        final RawMessagingPreprocessor result = fixture.getRawMessagePreprocessor();
        assertNotNull(result);
        assertTrue(result instanceof NoOpRawMessagingPreprocessor);
    }

    @Test
    public void testGetMqttClientIdProvider() throws Exception {
        createFixture();
        final MqttClientIdProvider result = fixture.getMqttClientIdProvider();
        assertNotNull(result);
        //noinspection ConstantValue
        assertTrue(result instanceof MqttClientIdProvider);
    }

    @Test
    public void testJoynrMessageProcessorAdded() throws Exception {
        createFixture();
        final Injector injector = fixture.getInjector();
        final List<Binding<JoynrMessageProcessor>> bindings = injector.findBindingsByType(new TypeLiteral<>() {
        });
        assertEquals(1, bindings.size());
    }

    @Test
    public void testJoynrMessageProcessorUsed() throws Exception {
        createFixture();
        final Injector injector = fixture.getInjector();
        final MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
        final MutableMessage request = messageFactory.createRequest("from",
                                                                    "to",
                                                                    new Request("name", new Object[0], new Class[0]),
                                                                    new MessagingQos());
        assertEquals("test", request.getCustomHeaders().get("test"));
    }

    @Test
    public void testClusterableParticipantIdsAdded() throws Exception {
        createFixture();
        final JoynrRuntime joynrRuntime = fixture.create(new HashSet<>(List.of(MyServiceSync.class)));
        assertNotNull(joynrRuntime);
        final Properties properties = fixture.getInjector()
                                             .getInstance(Key.get(Properties.class,
                                                                  Names.named(MessagingPropertyKeys.JOYNR_PROPERTIES)));
        assertNotNull(properties);
        final String key = (ParticipantIdKeyUtil.JOYNR_PARTICIPANT_PREFIX
                + LOCAL_DOMAIN + "." + MyService.INTERFACE_NAME + ".v"
                + ProviderAnnotations.getMajorVersion(MyServiceProvider.class)).toLowerCase().replace("/", ".");
        assertTrue(properties.containsKey(key));
        final String value = properties.getProperty(key);
        assertNotNull(value);
        assertEquals((LOCAL_DOMAIN + "." + CHANNEL_ID + "." + MyService.INTERFACE_NAME + ".v"
                + ProviderAnnotations.getMajorVersion(MyServiceProvider.class)).replace("/", "."), value);
    }

    @Test
    public void testNoOverrideForManuallyAddedParticipantIds() throws Exception {
        final Properties joynrProperties = new Properties();
        final String key = (ParticipantIdKeyUtil.JOYNR_PARTICIPANT_PREFIX + LOCAL_DOMAIN + "."
                + MyService.INTERFACE_NAME).toLowerCase().replace("/", ".");
        joynrProperties.setProperty(key, "myvalue");
        createFixture(joynrProperties);

        final JoynrRuntime joynrRuntime = fixture.create(new HashSet<>(List.of(MyServiceSync.class)));
        assertNotNull(joynrRuntime);
        final Properties properties = fixture.getInjector()
                                             .getInstance(Key.get(Properties.class,
                                                                  Names.named(MessagingPropertyKeys.JOYNR_PROPERTIES)));
        assertNotNull(properties);
        assertTrue(properties.containsKey(key));
        final String value = properties.getProperty(key);
        assertNotNull(value);
        assertEquals("myvalue", value);
    }

    @Test
    public void testCreateWithAccessControlEnabled() throws Exception {
        final Properties properties = new Properties();
        properties.put(ClusterControllerRuntimeModule.PROPERTY_ACCESSCONTROL_ENABLE, "true");
        createFixture(properties);
        final JoynrRuntime joynrRuntime = fixture.create(new HashSet<>(List.of(MyServiceSync.class)));
        assertNotNull(joynrRuntime);
    }

    @Test
    public void testJoynrLocalDomainUnsatisfiedThrows() throws Exception {
        final Instance<String> joynrLocalDomain = createLocalDomainMock();
        when(joynrLocalDomain.isUnsatisfied()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_NO_LOCAL_DOMAIN);
        createFixture(createPropertiesMock(null), joynrLocalDomain);
    }

    @Test
    public void testJoynrLocalDomainAmbiguousThrows() throws Exception {
        final Instance<String> joynrLocalDomain = createLocalDomainMock();
        when(joynrLocalDomain.isAmbiguous()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_MULTIPLE_LOCAL_DOMAINS);
        createFixture(createPropertiesMock(null), joynrLocalDomain);
    }

    @Test
    public void testCreateRuntimeFactoryFailsIfLocalDomainIsNull() throws Exception {
        final Instance<String> joynrLocalDomain = createLocalDomainMock();
        when(joynrLocalDomain.isAmbiguous()).thenReturn(false);
        when(joynrLocalDomain.isUnsatisfied()).thenReturn(false);
        when(joynrLocalDomain.get()).thenReturn(null);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_LOCAL_DOMAIN_IS_EMPTY);
        createFixture(createPropertiesMock(null), joynrLocalDomain);
    }

    @Test
    public void testCreateRuntimeFactoryFailsIfLocalDomainIsEmpty() throws Exception {
        final Instance<String> joynrLocalDomain = createLocalDomainMock();
        when(joynrLocalDomain.isAmbiguous()).thenReturn(false);
        when(joynrLocalDomain.isUnsatisfied()).thenReturn(false);
        when(joynrLocalDomain.get()).thenReturn("");

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_LOCAL_DOMAIN_IS_EMPTY);
        createFixture(createPropertiesMock(null), joynrLocalDomain);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCreateRuntimeFactoryFailsIfMultipleRawMessagePreProcessorsProvided() throws Exception {
        final Instance<RawMessagingPreprocessor> rawMessageProcessor = mock(Instance.class);
        when(rawMessageProcessor.isAmbiguous()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_MULTIPLE_PREPROCESSORS);
        createFixtureWithPreProcessor(rawMessageProcessor);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCreateRuntimeFactorySetsNoOpOreProcessorIfNoSpecified() throws Exception {
        final Instance<RawMessagingPreprocessor> rawMessageProcessor = mock(Instance.class);
        when(rawMessageProcessor.isUnsatisfied()).thenReturn(true);

        createFixtureWithPreProcessor(rawMessageProcessor);

        assertNotNull(fixture);
        final Field field = DefaultJoynrRuntimeFactory.class.getDeclaredField("rawMessagePreprocessor");
        field.setAccessible(true);
        final Object preProcessor = field.get(fixture);
        assertNotNull(preProcessor);
        assertTrue(preProcessor instanceof NoOpRawMessagingPreprocessor);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCreateRuntimeFactoryFailsIfMultipleMqttClientIdProvidersProvided() throws Exception {
        final Instance<MqttClientIdProvider> mqttClientIdProvider = mock(Instance.class);
        when(mqttClientIdProvider.isAmbiguous()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_MULTIPLE_ID_CLIENTS);
        createFixtureWithMqttClientIdProvider(mqttClientIdProvider);
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testCreateRuntimeFactorySetsNullIfNoMqttClientIdProviderProvided() throws Exception {
        final Instance<MqttClientIdProvider> mqttClientIdProvider = mock(Instance.class);
        when(mqttClientIdProvider.isUnsatisfied()).thenReturn(true);

        createFixtureWithMqttClientIdProvider(mqttClientIdProvider);

        assertNotNull(fixture);
        final Field field = DefaultJoynrRuntimeFactory.class.getDeclaredField("mqttClientIdProvider");
        field.setAccessible(true);
        final Object preProcessor = field.get(fixture);
        assertNull(preProcessor);
    }

    @Test
    public void testJoynrPropertiesUnsatisfiedDoesNotThrow() throws Exception {
        final Instance<Properties> joynrProperties = createPropertiesMock(null);
        when(joynrProperties.isUnsatisfied()).thenReturn(true);

        createFixture(joynrProperties, createLocalDomainMock());
    }

    @Test
    public void testJoynrPropertiesAmbiguousThrows() throws Exception {
        final Instance<Properties> joynrProperties = createPropertiesMock(null);
        when(joynrProperties.isAmbiguous()).thenReturn(true);

        expectedException.expect(JoynrIllegalStateException.class);
        expectedException.expectMessage(ERROR_MULTIPLE_PROPERTIES);
        createFixture(joynrProperties, createLocalDomainMock());
    }

    @Test
    public void testSharedSubscriptionsAreEnabledByDefault() throws Exception {
        createFixture();

        final boolean sharedSubscriptionValue = getSharedSubscriptionOption();
        assertTrue(sharedSubscriptionValue);
    }

    @Test
    public void testSharedSubscriptionsAreDisabledWhenInPropertiesThisOptionWasDisabled() throws Exception {
        final Properties properties = new Properties();
        properties.setProperty(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "false");
        createFixture(properties);

        final boolean sharedSubscriptionValue = getSharedSubscriptionOption();
        assertFalse(sharedSubscriptionValue);
    }

    @Test
    public void testSharedSubscriptionsAreEnabledWhenInPropertiesThisOptionWasEnabled() throws Exception {
        final Properties properties = new Properties();
        properties.setProperty(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS, "true");
        createFixture(properties);

        final boolean sharedSubscriptionValue = getSharedSubscriptionOption();
        assertTrue(sharedSubscriptionValue);
    }

    private boolean getSharedSubscriptionOption() throws NoSuchFieldException, IllegalAccessException {
        final Properties joynrProperties = extractProperties();
        return Boolean.parseBoolean((String) joynrProperties.get(PROPERTY_KEY_MQTT_ENABLE_SHARED_SUBSCRIPTIONS));
    }

    private Properties extractProperties() throws NoSuchFieldException, IllegalAccessException {
        final Field field = DefaultJoynrRuntimeFactory.class.getDeclaredField("joynrProperties");
        field.setAccessible(true);
        return (Properties) field.get(fixture);
    }
}
