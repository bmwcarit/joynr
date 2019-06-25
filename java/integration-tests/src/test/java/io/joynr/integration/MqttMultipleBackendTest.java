/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import static com.google.inject.util.Modules.override;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.JoynrVersion;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.dispatching.MutableMessageFactory;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.IMqttMessagingSkeleton;
import io.joynr.messaging.mqtt.JoynrMqttClient;
import io.joynr.messaging.mqtt.MqttClientFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonFactory;
import io.joynr.messaging.mqtt.MqttMessagingSkeletonProvider;
import io.joynr.messaging.mqtt.MqttModule;
import io.joynr.messaging.mqtt.paho.client.MqttPahoClientFactory;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.smrf.EncodingException;
import io.joynr.smrf.UnsuppportedVersionException;
import joynr.ImmutableMessage;
import joynr.Message;
import joynr.MutableMessage;
import joynr.Reply;
import joynr.exceptions.ApplicationException;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.DefaulttestProvider;
import joynr.tests.testProvider;
import joynr.tests.testProxy;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendTest {

    private final String TESTGBID1 = "testgbid1";
    private final String TESTGBID2 = "testgbid2";
    private final String[] gbids = new String[]{ TESTGBID1, TESTGBID2 };
    private final String TESTDOMAIN = "testDomain";
    private final String TESTTOPIC = "testTopic";

    private Properties properties;
    private Injector injectorWithMockedGcdClient;
    private Injector injector;
    private JoynrRuntime joynrRuntimeWithMockedGcdClient;
    private JoynrRuntime joynrRuntime;

    private GlobalDiscoveryEntry globalDiscoveryEntry1;
    private GlobalDiscoveryEntry globalDiscoveryEntry2;
    private DiscoveryQos discoveryQos;
    private ProviderQos providerQos;

    @Mock
    private GlobalCapabilitiesDirectoryClient gcdClient;

    @Mock
    private MqttPahoClientFactory mqttPahoClientFactory;

    @Mock
    private JoynrMqttClient joynrMqttClient1;

    @Mock
    private JoynrMqttClient joynrMqttClient2;

    @Before
    public void setUp() {
        JoynrVersion joynrVersion = testProxy.class.getAnnotation(JoynrVersion.class);

        doReturn(joynrMqttClient1).when(mqttPahoClientFactory).createReceiver(TESTGBID1);
        doReturn(joynrMqttClient1).when(mqttPahoClientFactory).createSender(TESTGBID1);
        doReturn(joynrMqttClient2).when(mqttPahoClientFactory).createReceiver(TESTGBID2);
        doReturn(joynrMqttClient2).when(mqttPahoClientFactory).createSender(TESTGBID2);

        properties = createProperties(TESTGBID1 + ", " + TESTGBID2, "tcp://localhost:1883, tcp://otherhost:1883");

        globalDiscoveryEntry1 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry1.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry1.setParticipantId("participantId1");
        globalDiscoveryEntry1.setDomain(TESTDOMAIN);

        globalDiscoveryEntry2 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry2.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry2.setParticipantId("participantId2");
        globalDiscoveryEntry2.setDomain(TESTDOMAIN);

        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(30000);
        discoveryQos.setRetryIntervalMs(discoveryQos.getDiscoveryTimeoutMs() + 1); // no retry
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);
    }

    @After
    public void tearDown() {
        if (joynrRuntimeWithMockedGcdClient != null) {
            joynrRuntimeWithMockedGcdClient.shutdown(true);
        }
        if (joynrRuntime != null) {
            joynrRuntime.shutdown(true);
        }
    }

    private Properties createProperties(String gbids, String brokerUris) {
        Properties properties = new Properties();
        properties.put(MqttModule.PROPERTY_MQTT_BROKER_URIS, brokerUris);
        properties.put(MqttModule.PROPERTY_KEY_MQTT_KEEP_ALIVE_TIMERS_SEC, "60,30");
        properties.put(MqttModule.PROPERTY_KEY_MQTT_CONNECTION_TIMEOUTS_SEC, "60,30");
        properties.put(ConfigurableMessagingSettings.PROPERTY_GBIDS, gbids);
        return properties;
    }

    private void addAddressesToGlobalDiscoveryEntries() {
        // has to be done after creating a JoynrRuntime with Guice which injects the objectMapper to CapabilitiesUtils
        MqttAddress mqttAddress1 = new MqttAddress(TESTGBID1, TESTTOPIC);
        globalDiscoveryEntry1.setAddress(CapabilityUtils.serializeAddress(mqttAddress1));

        MqttAddress mqttAddress2 = new MqttAddress(TESTGBID2, TESTTOPIC);
        globalDiscoveryEntry2.setAddress(CapabilityUtils.serializeAddress(mqttAddress2));
    }

    private void createJoynrRuntimeWithMockedGcdClient() {
        injectorWithMockedGcdClient = Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                                                    new MqttPahoModule()).with(new JoynrPropertiesModule(properties),
                                                                                               new AbstractModule() {
                                                                                                   @Override
                                                                                                   protected void configure() {
                                                                                                       bind(MqttClientFactory.class).toInstance(mqttPahoClientFactory);
                                                                                                       bind(GlobalCapabilitiesDirectoryClient.class).toInstance(gcdClient);
                                                                                                       bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                                                                           .toInstance(gbids);
                                                                                                   }
                                                                                               }));
        joynrRuntimeWithMockedGcdClient = injectorWithMockedGcdClient.getInstance(JoynrRuntime.class);
        addAddressesToGlobalDiscoveryEntries();
    }

    private void createJoynrRuntime() {
        injector = Guice.createInjector(override(new CCInProcessRuntimeModule(),
                                                 new MqttPahoModule()).with(new JoynrPropertiesModule(properties),
                                                                            new AbstractModule() {
                                                                                @Override
                                                                                protected void configure() {
                                                                                    bind(MqttClientFactory.class).toInstance(mqttPahoClientFactory);
                                                                                    bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                                                        .toInstance(gbids);
                                                                                }
                                                                            }));
        joynrRuntime = injector.getInstance(JoynrRuntime.class);
        addAddressesToGlobalDiscoveryEntries();
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProxyCall() throws InterruptedException {
        createJoynrRuntimeWithMockedGcdClient();
        ArgumentCaptor<String> topicCaptor = ArgumentCaptor.forClass(String.class);
        testProxy proxy1 = buildProxyForGlobalDiscoveryEntry(globalDiscoveryEntry1);
        testProxy proxy2 = buildProxyForGlobalDiscoveryEntry(globalDiscoveryEntry2);
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());

        Semaphore publishSemaphore = new Semaphore(0);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                publishSemaphore.release();
                return null;
            }
        }).when(joynrMqttClient1).publishMessage(anyString(), any(byte[].class), anyInt());
        proxy1.methodFireAndForgetWithoutParams();
        assertTrue(publishSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient1).publishMessage(topicCaptor.capture(), any(byte[].class), anyInt());
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC));
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());

        reset(joynrMqttClient1);
        reset(joynrMqttClient2);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) {
                publishSemaphore.release();
                return null;
            }
        }).when(joynrMqttClient2).publishMessage(anyString(), any(byte[].class), anyInt());
        proxy2.methodFireAndForgetWithoutParams();
        assertTrue(publishSemaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient1, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(joynrMqttClient2).publishMessage(topicCaptor.capture(), any(byte[].class), anyInt());
        assertTrue(topicCaptor.getValue().startsWith(TESTTOPIC));
    }

    private testProxy buildProxyForGlobalDiscoveryEntry(GlobalDiscoveryEntry globalDiscoveryEntry) throws InterruptedException {
        Semaphore semaphore = new Semaphore(0);

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError> callback = (CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>) invocation.getArguments()[0];
                List<GlobalDiscoveryEntry> globalDiscoveryEntryList = new ArrayList<>();
                globalDiscoveryEntryList.add(globalDiscoveryEntry);
                callback.onSuccess(globalDiscoveryEntryList);
                return null;
            }
        }).when(gcdClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                  any(String[].class),
                                  anyString(),
                                  anyLong(),
                                  any(String[].class));

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntimeWithMockedGcdClient.getProxyBuilder(TESTDOMAIN,
                                                                                               testProxy.class);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        testProxy proxy = proxyBuilder.build(new ProxyCreatedCallback<testProxy>() {

            @Override
            public void onProxyCreationFinished(testProxy result) {
                semaphore.release();

            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("Proxy creation failed: " + error.toString());

            }
        });
        assertTrue(semaphore.tryAcquire(10, TimeUnit.SECONDS));
        reset(gcdClient);
        return proxy;
    }

    private String getGcdTopic(Injector injector) {
        GlobalDiscoveryEntry gcdDiscoveryEntry = injector.getInstance(Key.get(GlobalDiscoveryEntry.class,
                                                                              Names.named(MessagingPropertyKeys.CAPABILITIES_DIRECTORY_DISCOVERY_ENTRY)));
        String gcdTopic = ((MqttAddress) CapabilityUtils.getAddressFromGlobalDiscoveryEntry(gcdDiscoveryEntry)).getTopic();
        gcdTopic += "/low/" + gcdDiscoveryEntry.getParticipantId();
        return gcdTopic;
    }

    private void testCorrectBackendIsContactedForLookup(String[] gbidsForLookup,
                                                        JoynrMqttClient expectedClient,
                                                        JoynrMqttClient otherClient) throws InterruptedException {
        createJoynrRuntime();
        String gcdTopic = getGcdTopic(injector);

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                publishCountDownLatch.countDown();
                return null;
            }
        }).when(expectedClient).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTDOMAIN, testProxy.class);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        if (gbidsForLookup != null) {
            proxyBuilder.setGbids(gbidsForLookup);
        }
        proxyBuilder.build();

        assertTrue(publishCountDownLatch.await(500, TimeUnit.MILLISECONDS));
        verify(otherClient, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(expectedClient).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_noGbids() throws InterruptedException {
        testCorrectBackendIsContactedForLookup(null, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_singleDefaultGbid() throws InterruptedException {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID1 }, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_singleNonDefaultGbid() throws InterruptedException {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID2 }, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_multipleGbids() throws InterruptedException {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID1, TESTGBID2 },
                                               joynrMqttClient1,
                                               joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForLookup_multipleGbidsReversed() throws InterruptedException {
        testCorrectBackendIsContactedForLookup(new String[]{ TESTGBID2, TESTGBID1 },
                                               joynrMqttClient2,
                                               joynrMqttClient1);
    }

    private void testCorrectBackendIsContactedForAdd(String[] gbidsForAdd,
                                                     JoynrMqttClient expectedClient,
                                                     JoynrMqttClient otherClient) throws InterruptedException {
        createJoynrRuntime();
        String gcdTopic = getGcdTopic(injector);

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                publishCountDownLatch.countDown();
                return null;
            }
        }).when(expectedClient).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());

        if (gbidsForAdd == null) {
            joynrRuntime.registerProvider(TESTDOMAIN, new DefaulttestProvider(), providerQos, true);
        } else {
            joynrRuntime.registerProvider(TESTDOMAIN, new DefaulttestProvider(), providerQos, gbidsForAdd, true);
        }

        assertTrue(publishCountDownLatch.await(500, TimeUnit.MILLISECONDS));
        verify(otherClient, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(expectedClient).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_noGbids() throws InterruptedException {
        testCorrectBackendIsContactedForAdd(null, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_singleDefaultGbid() throws InterruptedException {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID1 }, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_singleNonDefaultGbid() throws InterruptedException {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID2 }, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_multipleGbids() throws InterruptedException {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID1, TESTGBID2 }, joynrMqttClient1, joynrMqttClient2);
    }

    @Test
    public void testCorrectBackendIsContactedForAdd_multipleGbidsReversed() throws InterruptedException {
        testCorrectBackendIsContactedForAdd(new String[]{ TESTGBID2, TESTGBID1 }, joynrMqttClient2, joynrMqttClient1);
    }

    @Test
    public void testCorrectBackendIsContactedForAddToAll() throws InterruptedException {
        createJoynrRuntime();
        String gcdTopic = getGcdTopic(injector);

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                publishCountDownLatch.countDown();
                return null;
            }
        }).when(joynrMqttClient1).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());

        joynrRuntime.registerInAllKnownBackends(TESTDOMAIN, new DefaulttestProvider(), providerQos, true);

        assertTrue(publishCountDownLatch.await(500, TimeUnit.MILLISECONDS));
        verify(joynrMqttClient2, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        verify(joynrMqttClient1).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());
    }

    private void testLookupWithDiscoveryError(String[] gbidsForLookup, DiscoveryError expectedError) {
        createJoynrRuntimeWithMockedGcdClient();

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntimeWithMockedGcdClient.getProxyBuilder(TESTDOMAIN,
                                                                                               testProxy.class);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        proxyBuilder.setGbids(gbidsForLookup);
        testProxy proxy = proxyBuilder.build();

        try {
            proxy.voidOperation();
            fail("Should never get this far.");
        } catch (DiscoveryException e) {
            String errorMsg = e.getMessage();
            assertNotNull(errorMsg);
            assertTrue("Error message does not contain \"DiscoveryError\": " + errorMsg,
                       errorMsg.contains("DiscoveryError"));
            assertTrue("Error message does not contain \"" + expectedError + "\": " + errorMsg,
                       errorMsg.contains(expectedError.name()));
        }
    }

    private void checkGcdClientAddLookupNotCalled() {
        verify(gcdClient, times(0)).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                                        any(GlobalDiscoveryEntry.class),
                                        any(String[].class));
        verify(gcdClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<GlobalDiscoveryEntry, DiscoveryError>> any(),
                                any(String.class),
                                anyLong(),
                                any(String[].class));
        verify(gcdClient,
               times(0)).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                any(String[].class),
                                any(String.class),
                                anyLong(),
                                any(String[].class));
    }

    @Test
    public void testLookupWithUnknownGbid_singleGbid() {
        testLookupWithDiscoveryError(new String[]{ "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithUnknownGbid_multipleGbids() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_singleGbid_null() {
        testLookupWithDiscoveryError(new String[]{ null }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_singleGbid_empty() {
        testLookupWithDiscoveryError(new String[]{ "" }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_multipleGbids_null() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, null }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_multipleGbids_empty() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, "" }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testLookupWithInvalidGbid_multipleGbids_duplicate() {
        testLookupWithDiscoveryError(new String[]{ TESTGBID1, TESTGBID2, TESTGBID1 }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    private void testLookupWithGlobalDiscoveryError(DiscoveryError expectedError, String[] gbidsForLookup) {
        final String[] expectedGbids = gbidsForLookup.clone();

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError> callback = (CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>) invocation.getArguments()[0];
                callback.onFailure(expectedError);
                return null;
            }
        }).when(gcdClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                  any(String[].class),
                                  anyString(),
                                  anyLong(),
                                  any(String[].class));

        testLookupWithDiscoveryError(gbidsForLookup, expectedError);
        verify(gcdClient).lookup(Matchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                 eq(new String[]{ TESTDOMAIN }),
                                 eq(testProxy.INTERFACE_NAME),
                                 anyLong(),
                                 eq(expectedGbids));
    }

    @Test
    public void testLookupWithGloballyInvalidGbid() {
        final DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    @Test
    public void testLookupWithGloballyUnknownGbid() {
        final DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    @Test
    public void testLookupWithGcdInternalError() {
        final DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    @Test
    public void testLookupForProviderInDifferentBackend() {
        final DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS;
        final String[] gbidsForLookup = new String[]{ TESTGBID1 };
        testLookupWithGlobalDiscoveryError(expectedError, gbidsForLookup);
    }

    private void testAddWithDiscoveryError(String[] gbidsForAdd, DiscoveryError expectedError) {
        createJoynrRuntimeWithMockedGcdClient();

        Future<Void> registerFuture = joynrRuntimeWithMockedGcdClient.registerProvider(TESTDOMAIN,
                                                                                       new DefaulttestProvider(),
                                                                                       providerQos,
                                                                                       gbidsForAdd,
                                                                                       true);

        try {
            registerFuture.get(10000);
            fail("Should never get this far.");
        } catch (ApplicationException e) {
            assertEquals(expectedError, e.getError());
        } catch (Exception e) {
            fail("Unexpected exception from registerProvider: " + e);
        }
    }

    private void testAddWithIllegalArgumentException(String[] gbidsForAdd) {
        createJoynrRuntimeWithMockedGcdClient();

        try {
            joynrRuntimeWithMockedGcdClient.registerProvider(TESTDOMAIN,
                                                             new DefaulttestProvider(),
                                                             providerQos,
                                                             gbidsForAdd,
                                                             true);
            fail("Should never get this far.");
        } catch (IllegalArgumentException e) {
            assertEquals("Provided gbid value(s) must not be empty!", e.getMessage());
        } catch (Exception e) {
            fail("Unexpected exception from registerProvider: " + e);
        }
    }

    @Test
    public void testAddWithUnknownGbid_singleGbid() {
        testAddWithDiscoveryError(new String[]{ "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithUnknownGbid_multipleGbids() {
        testAddWithDiscoveryError(new String[]{ TESTGBID1, "unknownGbid" }, DiscoveryError.UNKNOWN_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_singleGbid_null() {
        testAddWithIllegalArgumentException(new String[]{ null });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_singleGbid_empty() {
        testAddWithIllegalArgumentException(new String[]{ "" });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_multipleGbids_null() {
        testAddWithIllegalArgumentException(new String[]{ TESTGBID1, null });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_multipleGbids_empty() {
        testAddWithIllegalArgumentException(new String[]{ TESTGBID1, "" });
        checkGcdClientAddLookupNotCalled();
    }

    @Test
    public void testAddWithInvalidGbid_multipleGbids_duplicate() {
        testAddWithDiscoveryError(new String[]{ TESTGBID1, TESTGBID2, TESTGBID1 }, DiscoveryError.INVALID_GBID);
        checkGcdClientAddLookupNotCalled();
    }

    private void testAddWithGlobalDiscoveryError(DiscoveryError expectedError, String[] gbidsForAdd) {
        final String[] expectedGbids = gbidsForAdd.clone();

        doAnswer(new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                CallbackWithModeledError<Void, DiscoveryError> callback = (CallbackWithModeledError<Void, DiscoveryError>) invocation.getArguments()[0];
                callback.onFailure(expectedError);
                return null;
            }
        }).when(gcdClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                               any(GlobalDiscoveryEntry.class),
                               any(String[].class));

        testAddWithDiscoveryError(gbidsForAdd, expectedError);

        ArgumentCaptor<GlobalDiscoveryEntry> gdeCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        verify(gcdClient).add(Matchers.<CallbackWithModeledError<Void, DiscoveryError>> any(),
                              gdeCaptor.capture(),
                              eq(expectedGbids));
        assertEquals(TESTDOMAIN, gdeCaptor.getValue().getDomain());
        assertEquals(testProxy.INTERFACE_NAME, gdeCaptor.getValue().getInterfaceName());
    }

    @Test
    public void testAddWithGloballyInvalidGbid() {
        final DiscoveryError expectedError = DiscoveryError.INVALID_GBID;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

    @Test
    public void testAddWithGloballyUnknownGbid() {
        final DiscoveryError expectedError = DiscoveryError.UNKNOWN_GBID;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

    @Test
    public void testAddWithGcdInternalError() {
        final DiscoveryError expectedError = DiscoveryError.INTERNAL_ERROR;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

    @Test
    public void testAddForUnavailableProvider() {
        final DiscoveryError expectedError = DiscoveryError.NO_ENTRY_FOR_SELECTED_BACKENDS;
        final String[] gbidsForAdd = new String[]{ TESTGBID1 };
        testAddWithGlobalDiscoveryError(expectedError, gbidsForAdd);
    }

    private void fakeVoidReply(String targetGbid,
                               Injector injector,
                               byte[] serializedRequestMessage) throws EncodingException, UnsuppportedVersionException {
        ImmutableMessage requestMessage = new ImmutableMessage(serializedRequestMessage);
        String requestReplyId = requestMessage.getCustomHeaders().get(Message.CUSTOM_HEADER_REQUEST_REPLY_ID);
        MutableMessageFactory messageFactory = injector.getInstance(MutableMessageFactory.class);
        MutableMessage replyMessage = messageFactory.createReply(requestMessage.getRecipient(),
                                                                 requestMessage.getSender(),
                                                                 new Reply(requestReplyId),
                                                                 new MessagingQos());

        MqttMessagingSkeletonProvider skeletonProvider = injector.getInstance(MqttMessagingSkeletonProvider.class);
        MqttMessagingSkeletonFactory skeletonFactory = (MqttMessagingSkeletonFactory) skeletonProvider.get();
        IMqttMessagingSkeleton skeleton = (IMqttMessagingSkeleton) skeletonFactory.getSkeleton(new MqttAddress(targetGbid,
                                                                                                               ""));
        skeleton.transmit(replyMessage.getImmutableMessage().getSerializedMessage(), new FailureAction() {
            @Override
            public void execute(Throwable error) {
                fail("fake reply failed in skeleton.transmit: " + error);
            }
        });
    }

    private testProvider registerProvider(String[] targetGbids,
                                          JoynrMqttClient expectedClient,
                                          JoynrMqttClient otherClient,
                                          Injector injector) throws InterruptedException, EncodingException,
                                                             UnsuppportedVersionException {
        final String gcdTopic = getGcdTopic(injector);

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                publishCountDownLatch.countDown();
                return null;
            }
        }).when(expectedClient).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());

        testProvider provider = new DefaulttestProvider();
        Future<Void> future;
        if (targetGbids == null) {
            future = joynrRuntime.registerProvider(TESTDOMAIN, provider, providerQos, true);
        } else {
            future = joynrRuntime.registerProvider(TESTDOMAIN, provider, providerQos, targetGbids, true);
        }

        assertTrue(publishCountDownLatch.await(500, TimeUnit.MILLISECONDS));
        verify(otherClient, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(expectedClient).publishMessage(eq(gcdTopic), messageCaptor.capture(), anyInt());

        reset(expectedClient);

        fakeVoidReply(targetGbids == null ? gbids[0] : targetGbids[0], injector, messageCaptor.getValue());

        try {
            future.get(10000);
        } catch (Exception e) {
            fail("registerProvider failed: " + e);
        }
        return provider;
    }

    private void unregisterProvider(Object provider,
                                    String targetGbid,
                                    JoynrMqttClient expectedClient,
                                    JoynrMqttClient otherClient,
                                    JoynrRuntime runtime,
                                    Injector injector) throws InterruptedException, EncodingException,
                                                       UnsuppportedVersionException {
        final String gcdTopic = getGcdTopic(injector);

        CountDownLatch publishCountDownLatch = new CountDownLatch(1);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                publishCountDownLatch.countDown();
                return null;
            }
        }).when(expectedClient).publishMessage(eq(gcdTopic), any(byte[].class), anyInt());

        runtime.unregisterProvider(TESTDOMAIN, provider);

        assertTrue(publishCountDownLatch.await(500, TimeUnit.MILLISECONDS));
        verify(otherClient, times(0)).publishMessage(anyString(), any(byte[].class), anyInt());
        ArgumentCaptor<byte[]> messageCaptor = ArgumentCaptor.forClass(byte[].class);
        verify(expectedClient).publishMessage(eq(gcdTopic), messageCaptor.capture(), anyInt());

        reset(expectedClient);

        fakeVoidReply(targetGbid, injector, messageCaptor.getValue());
        // wait for delivery
        Thread.sleep(100);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInSelectedDefaultBackend() throws InterruptedException,
                                                                                            EncodingException,
                                                                                            UnsuppportedVersionException {
        final String expectedGbid = TESTGBID1;
        final String[] targetGbids = new String[]{ expectedGbid };
        createJoynrRuntime();
        testProvider provider = registerProvider(targetGbids, joynrMqttClient1, joynrMqttClient2, injector);
        unregisterProvider(provider, expectedGbid, joynrMqttClient1, joynrMqttClient2, joynrRuntime, injector);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInNonSelectedDefaultBackend() throws InterruptedException,
                                                                                               EncodingException,
                                                                                               UnsuppportedVersionException {
        final String expectedGbid = TESTGBID1;
        final String[] targetGbids = null; // no selection (old API)
        createJoynrRuntime();
        testProvider provider = registerProvider(targetGbids, joynrMqttClient1, joynrMqttClient2, injector);
        unregisterProvider(provider, expectedGbid, joynrMqttClient1, joynrMqttClient2, joynrRuntime, injector);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInNonDefaultBackend() throws InterruptedException,
                                                                                       EncodingException,
                                                                                       UnsuppportedVersionException {
        final String expectedGbid = TESTGBID2;
        final String[] targetGbids = new String[]{ expectedGbid };
        createJoynrRuntime();
        testProvider provider = registerProvider(targetGbids, joynrMqttClient2, joynrMqttClient1, injector);
        unregisterProvider(provider, expectedGbid, joynrMqttClient2, joynrMqttClient1, joynrRuntime, injector);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInMultipleBackends() throws InterruptedException,
                                                                                      EncodingException,
                                                                                      UnsuppportedVersionException {
        final String expectedGbid = TESTGBID1;
        final String[] targetGbids = new String[]{ expectedGbid, TESTGBID2 };
        createJoynrRuntime();
        testProvider provider = registerProvider(targetGbids, joynrMqttClient1, joynrMqttClient2, injector);
        unregisterProvider(provider, expectedGbid, joynrMqttClient1, joynrMqttClient2, joynrRuntime, injector);
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForRemove_providerInMultipleBackendsReversed() throws InterruptedException,
                                                                                              EncodingException,
                                                                                              UnsuppportedVersionException {
        final String expectedGbid = TESTGBID2;
        final String[] targetGbids = new String[]{ expectedGbid, TESTGBID1 };
        createJoynrRuntime();
        testProvider provider = registerProvider(targetGbids, joynrMqttClient2, joynrMqttClient1, injector);
        unregisterProvider(provider, expectedGbid, joynrMqttClient2, joynrMqttClient1, joynrRuntime, injector);
    }

}
