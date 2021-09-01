/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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
package io.joynr.integration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.Module;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientFactory;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.provider.AbstractJoynrProvider;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.Request;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.DefaulttestProvider;
import joynr.types.ProviderQos;

import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

public class MqttClientProviderIntegrationTest {

    private static final Logger logger = LoggerFactory.getLogger(MqttClientProviderIntegrationTest.class);

    private final long PROVIDER_REGISTRATION_TIMEOUT = 10000L;
    private final String TEST_DOMAIN = "MqttClientProviderIntegrationTest_Domain";
    private final String FIXED_PARTICIPANT_ID = "MqttClientProviderIntegrationTest_fixedParticipantId";
    private final String DEFAULT_GBID = "joynrdefaultgbid";

    private Injector injector;
    private Properties properties;
    private JoynrRuntime providerRuntime;
    private HivemqMqttClientFactory hivemqMqttClientFactory;
    private JoynrMqttClient sender;
    private MutableMessageFactory mutableMessageFactory;
    private TestProvider provider;

    private static class TestProvider extends DefaulttestProvider {

        private Map<String, String> customHeadersMap;

        private Semaphore semaphore = new Semaphore(0);

        public Map<String, String> getCustomHeaders() {
            return customHeadersMap;
        }

        @Override
        public Promise<DeferredVoid> voidOperation() {
            logger.info("###### voidOperation is called!!!");
            DeferredVoid deferred = new DeferredVoid();
            customHeadersMap = new HashMap<>();
            for (Map.Entry<String, Serializable> entry : AbstractJoynrProvider.getCallContext()
                                                                              .getContext()
                                                                              .entrySet()) {
                customHeadersMap.put(entry.getKey(), entry.getValue().toString());
            }
            deferred.resolve();
            semaphore.release();
            return new Promise<>(deferred);
        }

        public Semaphore getSemaphore() {
            return semaphore;
        }
    }

    private Injector createInjector(Properties properties) {
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule())
                                      .with(new HivemqMqttClientModule(), new JoynrPropertiesModule(properties));
        return Guice.createInjector(runtimeModule);
    }

    private ImmutableMessage createImmutableMessage(String toParticipantId,
                                                    Map<String, String> customHeaders) throws Exception {
        Request request = new Request("voidOperation", new Object[0], new Class[0]);
        MessagingQos messagingQos = new MessagingQos();
        messagingQos.putAllCustomMessageHeaders(customHeaders);
        MutableMessage requestMsg = mutableMessageFactory.createRequest("customSender",
                                                                        toParticipantId,
                                                                        request,
                                                                        messagingQos);
        return requestMsg.getImmutableMessage();
    }

    @Before
    public void setUp() throws Exception {
        properties = new Properties();
        String interfaceName = ProviderAnnotations.getInterfaceName(DefaulttestProvider.class);
        int majorVersion = ProviderAnnotations.getMajorVersion(DefaulttestProvider.class);
        properties.setProperty(ParticipantIdKeyUtil.getProviderParticipantIdKey(TEST_DOMAIN,
                                                                                interfaceName,
                                                                                majorVersion),
                               FIXED_PARTICIPANT_ID);
        properties.setProperty(ConfigurableMessagingSettings.PROPERTY_GBIDS, DEFAULT_GBID);

        injector = createInjector(properties);
        providerRuntime = injector.getInstance(JoynrRuntime.class);

        // register provider
        provider = spy(new TestProvider());
        providerRuntime.getProviderRegistrar(TEST_DOMAIN, provider)
                       .withProviderQos(new ProviderQos())
                       .awaitGlobalRegistration()
                       .register()
                       .get(PROVIDER_REGISTRATION_TIMEOUT);

        // get HivemqMqttFactory using injector
        hivemqMqttClientFactory = injector.getInstance(HivemqMqttClientFactory.class);

        // get mqtt client from hivemqMqttClientFactory
        sender = hivemqMqttClientFactory.createSender(DEFAULT_GBID);

        // inject mutable message factory
        mutableMessageFactory = injector.getInstance(MutableMessageFactory.class);
    }

    @Test
    public void customHeadersReceivedByProvider() throws Exception {
        // set expected custom headers
        final String customHeaderKey1 = "header1";
        final String customHeaderKey2 = "header2";
        final String customHeaderValue1 = "value1";
        final String customHeaderValue2 = "value2";
        Map<String, String> customHeaders = new HashMap<>();
        customHeaders.put(customHeaderKey1, customHeaderValue1);
        customHeaders.put(customHeaderKey2, customHeaderValue2);
        Map<String, String> expectedHeaders = new HashMap<>();
        customHeaders.forEach((k, v) -> expectedHeaders.put(k, v));
        CountDownLatch cdl = new CountDownLatch(1);

        // create an immutableMessage
        ImmutableMessage immutableMessage = createImmutableMessage(FIXED_PARTICIPANT_ID, customHeaders);
        expectedHeaders.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID,
                            immutableMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID));

        MqttAddress address = injector.getInstance(Key.get(MqttAddress.class,
                                                           Names.named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS)));

        sender.publishMessage(address.getTopic(),
                              immutableMessage.getSerializedMessage(),
                              immutableMessage.getPrefixedCustomHeaders(),
                              0,
                              60000,
                              new SuccessAction() {

                                  @Override
                                  public void execute() {
                                      cdl.countDown();
                                  }
                              },
                              new FailureAction() {

                                  @Override
                                  public void execute(Throwable error) {
                                      logger.error("#####PublishMessage failed!");

                                  }
                              });
        if (!cdl.await(1000, TimeUnit.MILLISECONDS)) {
            fail();
        }
        if (!provider.getSemaphore().tryAcquire(1000, TimeUnit.MILLISECONDS)) {
            fail();
        }
        verify(provider, times(1)).voidOperation();
        Map<String, String> receivedCustomHeadersMap = provider.getCustomHeaders();
        for (String key : expectedHeaders.keySet()) {
            assertEquals(expectedHeaders.get(key), receivedCustomHeadersMap.get(key));
        }
        assertEquals(expectedHeaders.size(), receivedCustomHeadersMap.size());
    }

    @Test
    public void extraCustomHeadersOverrideCustomHeaders() throws Exception {
        // set expected custom headers
        final String customHeaderKey1 = "header1";
        final String customHeaderKey2 = "header2";
        final String extraCustomHeaderKey = "header3";
        final String customHeaderValue1 = "value1";
        final String customHeaderValue1_2 = "value1_2";
        final String customHeaderValue2 = "value2";
        final String customHeaderValue2_2 = "value2_2";
        final String extraCustomHeaderValue = "value3";
        Map<String, String> originalCustomHeaders = new HashMap<>();
        originalCustomHeaders.put(customHeaderKey1, customHeaderValue1);
        originalCustomHeaders.put(customHeaderKey2, customHeaderValue2);
        Map<String, String> expectedHeaders = new HashMap<>();
        expectedHeaders.put(customHeaderKey1, customHeaderValue1);
        expectedHeaders.put(customHeaderKey2, customHeaderValue2_2);
        expectedHeaders.put(extraCustomHeaderKey, extraCustomHeaderValue);
        CountDownLatch cdl = new CountDownLatch(1);

        // create an immutableMessage
        ImmutableMessage immutableMessage = createImmutableMessage(FIXED_PARTICIPANT_ID, originalCustomHeaders);
        expectedHeaders.put(Message.CUSTOM_HEADER_REQUEST_REPLY_ID,
                            immutableMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID));

        Map<String, String> extraCustomHeaders = new HashMap<>();
        extraCustomHeaders.put(Message.CUSTOM_HEADER_PREFIX + customHeaderKey2, customHeaderValue2_2);
        extraCustomHeaders.put(Message.CUSTOM_HEADER_PREFIX + extraCustomHeaderKey, extraCustomHeaderValue);
        // add non prefixed extra custom header which will be dropped by the receiver
        extraCustomHeaders.put(customHeaderKey1, customHeaderValue1_2);

        MqttAddress address = injector.getInstance(Key.get(MqttAddress.class,
                                                           Names.named(MqttModule.PROPERTY_MQTT_GLOBAL_ADDRESS)));

        sender.publishMessage(address.getTopic(),
                              immutableMessage.getSerializedMessage(),
                              extraCustomHeaders,
                              0,
                              60000,
                              new SuccessAction() {

                                  @Override
                                  public void execute() {
                                      cdl.countDown();
                                  }
                              },
                              new FailureAction() {

                                  @Override
                                  public void execute(Throwable error) {
                                      logger.error("#####PublishMessage failed!");

                                  }
                              });
        if (!cdl.await(1000, TimeUnit.MILLISECONDS)) {
            fail();
        }
        if (!provider.getSemaphore().tryAcquire(1000, TimeUnit.MILLISECONDS)) {
            fail();
        }
        verify(provider, times(1)).voidOperation();
        Map<String, String> receivedCustomHeadersMap = provider.getCustomHeaders();
        for (String key : expectedHeaders.keySet()) {
            assertEquals(expectedHeaders.get(key), receivedCustomHeadersMap.get(key));
        }
        assertEquals(expectedHeaders.size(), receivedCustomHeadersMap.size());
    }
}
