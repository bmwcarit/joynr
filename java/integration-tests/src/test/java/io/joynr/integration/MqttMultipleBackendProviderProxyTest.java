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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Matchers;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.JoynrVersion;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.tests.testProxy;
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.Version;

/**
 * Test that the correct backend connection is used for (global) proxy calls and provider replies and publications.
 */
@RunWith(MockitoJUnitRunner.class)
public class MqttMultipleBackendProviderProxyTest extends AbstractMqttMultipleBackendTest {

    private GlobalDiscoveryEntry globalDiscoveryEntry1;
    private GlobalDiscoveryEntry globalDiscoveryEntry2;

    @Before
    public void setUp() {
        super.setUp();
        createJoynrRuntimeWithMockedGcdClient();

        JoynrVersion joynrVersion = testProxy.class.getAnnotation(JoynrVersion.class);

        globalDiscoveryEntry1 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry1.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry1.setParticipantId("participantId1");
        globalDiscoveryEntry1.setDomain(TESTDOMAIN);
        MqttAddress mqttAddress1 = new MqttAddress(TESTGBID1, TESTTOPIC);
        globalDiscoveryEntry1.setAddress(CapabilityUtils.serializeAddress(mqttAddress1));

        globalDiscoveryEntry2 = new GlobalDiscoveryEntry();
        globalDiscoveryEntry2.setProviderVersion(new Version(joynrVersion.major(), joynrVersion.minor()));
        globalDiscoveryEntry2.setParticipantId("participantId2");
        globalDiscoveryEntry2.setDomain(TESTDOMAIN);
        MqttAddress mqttAddress2 = new MqttAddress(TESTGBID2, TESTTOPIC);
        globalDiscoveryEntry2.setAddress(CapabilityUtils.serializeAddress(mqttAddress2));
    }

    @Test
    public void testCorrectMqttConnectionIsUsedForProxyMethodCall() throws InterruptedException {
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

        ProxyBuilder<testProxy> proxyBuilder = joynrRuntime.getProxyBuilder(TESTDOMAIN, testProxy.class);
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

}
