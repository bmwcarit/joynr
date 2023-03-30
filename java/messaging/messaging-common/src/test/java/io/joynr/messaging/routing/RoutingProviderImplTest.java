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
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.Properties;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrException;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MulticastReceiverRegistrar;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.util.ObjectMapper;
import joynr.exceptions.ProviderRuntimeException;
import joynr.system.RoutingSubscriptionPublisher;
import joynr.system.RoutingTypes.Address;
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
    private PromiseListener mockGlobalAddressPromiseListener;
    @Mock
    private PromiseListener mockReplyToAddressPromiseListener;
    @Mock
    private RoutingSubscriptionPublisher mockRoutingSubscriptionPublisher;

    private MqttAddress expectedMqttAddress;
    private MqttAddress expectedReplyToAddress;
    private RoutingProviderImpl routingProvider;

    @Before
    public void setUp() throws Exception {
        Field objectMapperField = RoutingTypesUtil.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(RoutingTypesUtil.class, new ObjectMapper());

        expectedMqttAddress = new MqttAddress("mqtt://test-broker-uri", "test-topic");
        expectedReplyToAddress = new MqttAddress("mqtt://test-broker-uri", "replyto-topic");

        Injector injector = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bind(MessageRouter.class).toInstance(mockMessageRouter);
                bind(MulticastReceiverRegistrar.class).toInstance(mockMulticastReceiverRegistrar);
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.GLOBAL_ADDRESS))
                                   .toInstance(expectedMqttAddress);
                bind(Address.class).annotatedWith(Names.named(MessagingPropertyKeys.REPLY_TO_ADDRESS))
                                   .toInstance(expectedReplyToAddress);
            }
        }, new JoynrPropertiesModule(new Properties()));
        routingProvider = injector.getInstance(RoutingProviderImpl.class);
        routingProvider.setSubscriptionPublisher(mockRoutingSubscriptionPublisher);
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

    @Test
    public void testGetGlobalAddress() {
        routingProvider.getGlobalAddress().then(mockGlobalAddressPromiseListener);
        verify(mockGlobalAddressPromiseListener).onFulfillment(RoutingTypesUtil.toAddressString(expectedMqttAddress));
    }

    @Test
    public void testGetReplyToAddress() {
        routingProvider.getReplyToAddress().then(mockGlobalAddressPromiseListener);
        verify(mockGlobalAddressPromiseListener).onFulfillment(RoutingTypesUtil.toAddressString(expectedReplyToAddress));
    }
}
