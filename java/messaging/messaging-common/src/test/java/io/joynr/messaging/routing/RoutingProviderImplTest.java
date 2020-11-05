/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.messaging.routing;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.ReplyToAddressProvider;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingSubscriptionPublisher;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.system.RoutingTypes.RoutingTypesUtil;
import joynr.system.RoutingTypes.UdsAddress;
import joynr.system.RoutingTypes.UdsClientAddress;

@RunWith(MockitoJUnitRunner.class)
public class RoutingProviderImplTest {

    @Mock
    private MessageRouter mockMessageRouter;
    @Mock
    private MulticastReceiverRegistrar mockMulticastReceiverRegistrar;
    @Mock
    private GlobalAddressProvider mockGlobalAddressProvider;
    @Mock
    private ReplyToAddressProvider mockReplyToAddressProvider;
    @Mock
    private PromiseListener mockGlobalAddressPromiseListener;
    @Mock
    private PromiseListener mockReplyToAddressPromiseListener;
    @Mock
    private RoutingSubscriptionPublisher mockRoutingSubscriptionPublisher;

    private MqttAddress expectedMqttAddress;
    private String expectedMqttAddressString;
    private ChannelAddress expectedChannelAddress;
    private String expectedChannelAddressString;
    private RoutingProviderImpl routingProvider;
    @Captor
    private ArgumentCaptor<TransportReadyListener> transportReadyListener;

    private Semaphore getGlobalAddressOnFulfillmentSemaphore;
    private Semaphore getReplyToAddressOnFulfillmentSemaphore;
    private Semaphore globalAddressChangedSemaphore;
    private Semaphore replyToAddressChangedSemaphore;

    @Before
    public void setUp() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());

        expectedMqttAddress = new MqttAddress("mqtt://test-broker-uri", "test-topic");
        expectedMqttAddressString = RoutingTypesUtil.toAddressString(expectedMqttAddress);
        expectedChannelAddress = new ChannelAddress("http://test-bounceproxy-url", "test-channelId");
        expectedChannelAddressString = RoutingTypesUtil.toAddressString(expectedChannelAddress);

        routingProvider = new RoutingProviderImpl(mockMessageRouter,
                                                  mockMulticastReceiverRegistrar,
                                                  mockGlobalAddressProvider,
                                                  mockReplyToAddressProvider);
        routingProvider.setSubscriptionPublisher(mockRoutingSubscriptionPublisher);

        getGlobalAddressOnFulfillmentSemaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                getGlobalAddressOnFulfillmentSemaphore.release();
                return null;
            }
        }).when(mockGlobalAddressPromiseListener).onFulfillment(anyString());

        globalAddressChangedSemaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                globalAddressChangedSemaphore.release();
                return null;
            }
        }).when(mockRoutingSubscriptionPublisher).globalAddressChanged(anyString());

        getReplyToAddressOnFulfillmentSemaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                getReplyToAddressOnFulfillmentSemaphore.release();
                return null;
            }
        }).when(mockReplyToAddressPromiseListener).onFulfillment(anyString());

        replyToAddressChangedSemaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                replyToAddressChangedSemaphore.release();
                return null;
            }
        }).when(mockRoutingSubscriptionPublisher).replyToAddressChanged(anyString());
    }

    @Test(timeout = 500)
    public void testGlobalAddressGetAfterTransportIsReady() throws InterruptedException {
        verify(mockGlobalAddressProvider).registerGlobalAddressesReadyListener(transportReadyListener.capture());
        transportReadyListener.getValue().transportReady(Optional.of(expectedMqttAddress));

        Promise<Deferred<String>> globalAddressPromise = routingProvider.getGlobalAddress();
        globalAddressPromise.then(mockGlobalAddressPromiseListener);
        verify(mockGlobalAddressPromiseListener).onFulfillment(expectedMqttAddressString);
        getGlobalAddressOnFulfillmentSemaphore.acquire();
    }

    @Test(timeout = 500)
    public void testReplyToAddressGetAfterTransportIsReady() throws InterruptedException {
        verify(mockReplyToAddressProvider).registerGlobalAddressesReadyListener(transportReadyListener.capture());
        transportReadyListener.getValue().transportReady(Optional.of(expectedMqttAddress));

        Promise<Deferred<String>> replyToAddressPromise = routingProvider.getReplyToAddress();
        replyToAddressPromise.then(mockReplyToAddressPromiseListener);
        verify(mockReplyToAddressPromiseListener).onFulfillment(expectedMqttAddressString);
        getReplyToAddressOnFulfillmentSemaphore.acquire();
    }

    @Test(timeout = 500)
    public void testGlobalAddressGetBeforeTransportIsReady() throws InterruptedException {
        Promise<Deferred<String>> globalAddressPromise = routingProvider.getGlobalAddress();

        verify(mockGlobalAddressProvider).registerGlobalAddressesReadyListener(transportReadyListener.capture());
        transportReadyListener.getValue().transportReady(Optional.of(expectedMqttAddress));

        globalAddressPromise.then(mockGlobalAddressPromiseListener);
        verify(mockGlobalAddressPromiseListener).onFulfillment(expectedMqttAddressString);
        getGlobalAddressOnFulfillmentSemaphore.acquire();
    }

    @Test(timeout = 500)
    public void testReplyToAddressGetBeforeTransportIsReady() throws InterruptedException {
        Promise<Deferred<String>> replyToAddressPromise = routingProvider.getReplyToAddress();

        verify(mockReplyToAddressProvider).registerGlobalAddressesReadyListener(transportReadyListener.capture());
        transportReadyListener.getValue().transportReady(Optional.of(expectedMqttAddress));

        replyToAddressPromise.then(mockReplyToAddressPromiseListener);
        verify(mockReplyToAddressPromiseListener).onFulfillment(expectedMqttAddressString);
        getReplyToAddressOnFulfillmentSemaphore.acquire();
    }

    @Test(timeout = 500)
    public void testGlobalAddressOnChangeNotifications() throws InterruptedException {
        verify(mockGlobalAddressProvider).registerGlobalAddressesReadyListener(transportReadyListener.capture());
        transportReadyListener.getValue().transportReady(Optional.of(expectedMqttAddress));

        verify(mockRoutingSubscriptionPublisher).globalAddressChanged(expectedMqttAddressString);

        transportReadyListener.getValue().transportReady(Optional.of(expectedChannelAddress));

        verify(mockRoutingSubscriptionPublisher).globalAddressChanged(expectedChannelAddressString);

        globalAddressChangedSemaphore.acquire(2);
    }

    @Test(timeout = 500)
    public void testReplyToAddressOnChangeNotifications() throws InterruptedException {
        verify(mockReplyToAddressProvider).registerGlobalAddressesReadyListener(transportReadyListener.capture());
        transportReadyListener.getValue().transportReady(Optional.of(expectedMqttAddress));

        verify(mockRoutingSubscriptionPublisher).replyToAddressChanged(expectedMqttAddressString);

        transportReadyListener.getValue().transportReady(Optional.of(expectedChannelAddress));

        verify(mockRoutingSubscriptionPublisher).replyToAddressChanged(expectedChannelAddressString);

        replyToAddressChangedSemaphore.acquire(2);
    }

    @Test
    public void addNextHop_udsAddress() throws InterruptedException {
        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final UdsAddress address = new UdsAddress();
        Promise<DeferredVoid> addNextHopPromise = routingProvider.addNextHop(participantId, address, isGloballyVisible);
        assertTrue(addNextHopPromise.isRejected());
        CountDownLatch cdl = new CountDownLatch(1);
        addNextHopPromise.then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException error) {
                assertTrue(error instanceof ProviderRuntimeException);
                assertTrue(((ProviderRuntimeException) error).getMessage().contains("not supported"));
                cdl.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("addNextHop succeeded");
            }
        });
        assertTrue(cdl.await(1, TimeUnit.SECONDS));
    }

    @Test
    public void addNextHop_udsClientAddress() throws InterruptedException {
        final String participantId = "testParticipantId";
        final boolean isGloballyVisible = true;
        final UdsClientAddress address = new UdsClientAddress();
        Promise<DeferredVoid> addNextHopPromise = routingProvider.addNextHop(participantId, address, isGloballyVisible);
        assertTrue(addNextHopPromise.isRejected());
        CountDownLatch cdl = new CountDownLatch(1);
        addNextHopPromise.then(new PromiseListener() {
            @Override
            public void onRejection(JoynrException error) {
                assertTrue(error instanceof ProviderRuntimeException);
                assertTrue(((ProviderRuntimeException) error).getMessage().contains("not supported"));
                cdl.countDown();
            }

            @Override
            public void onFulfillment(Object... values) {
                fail("addNextHop succeeded");
            }
        });
        assertTrue(cdl.await(1, TimeUnit.SECONDS));
    }
}
