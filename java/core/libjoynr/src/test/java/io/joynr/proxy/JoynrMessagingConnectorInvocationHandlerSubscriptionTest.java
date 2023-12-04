/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.proxy;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.invocation.AttributeSubscribeInvocation;
import io.joynr.proxy.invocation.BroadcastSubscribeInvocation;
import io.joynr.proxy.invocation.MulticastSubscribeInvocation;
import io.joynr.proxy.invocation.UnsubscribeInvocation;
import io.joynr.pubsub.SubscriptionQos;
import io.joynr.pubsub.subscription.AttributeSubscriptionAdapter;
import io.joynr.pubsub.subscription.AttributeSubscriptionListener;
import joynr.BroadcastFilterParameters;
import joynr.MulticastSubscriptionQos;
import joynr.OnChangeSubscriptionQos;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import java.lang.reflect.Method;
import java.util.Set;
import java.util.UUID;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;

@RunWith(MockitoJUnitRunner.class)
public class JoynrMessagingConnectorInvocationHandlerSubscriptionTest
        extends AbstractJoynrMessagingConnectorInvocationHandlerTest {

    @Test
    public void testUnsubscribingShouldSucceed() throws NoSuchMethodException {
        final String subscriptionId = UUID.randomUUID().toString();
        method = getSubscriptionMethod("unsubscribeFromTestAttribute", String.class);
        final UnsubscribeInvocation invocation = new UnsubscribeInvocation(method,
                                                                           new Object[]{ subscriptionId },
                                                                           null);

        handler.executeSubscriptionMethod(invocation);

        verify(subscriptionManager).unregisterSubscription(eq(FROM_PARTICIPANT_ID),
                                                           eq(toDiscoveryEntries),
                                                           eq(subscriptionId),
                                                           any(MessagingQos.class));
    }

    @Test
    public void testUnsubscribingShouldFailIfDiscoveryEntrySetIsEmpty() throws NoSuchMethodException {
        final String subscriptionId = UUID.randomUUID().toString();
        method = getSubscriptionMethod("unsubscribeFromTestAttribute", String.class);
        final UnsubscribeInvocation invocation = new UnsubscribeInvocation(method,
                                                                           new Object[]{ subscriptionId },
                                                                           null);
        toDiscoveryEntries.clear();
        try {
            handler.executeSubscriptionMethod(invocation);
        } catch (final JoynrIllegalStateException exception) {
            assertNotNull(exception);
            assertTrue(exception.getMessage().contains("at least one participant"));
            //noinspection unchecked
            verify(subscriptionManager, never()).unregisterSubscription(anyString(),
                                                                        any(Set.class),
                                                                        anyString(),
                                                                        any(MessagingQos.class));
        }
    }

    @Test
    public void testSubscribingToAttributeShouldSucceed() throws NoSuchMethodException {
        final AttributeSubscriptionAdapter<String> listener = new AttributeSubscriptionAdapter<>();
        final OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        method = getSubscriptionMethod("subscribeToTestAttribute",
                                       AttributeSubscriptionListener.class,
                                       SubscriptionQos.class);
        final AttributeSubscribeInvocation invocation = new AttributeSubscribeInvocation(method,
                                                                                         new Object[]{ listener,
                                                                                                 subscriptionQos },
                                                                                         null,
                                                                                         proxy);
        handler.executeSubscriptionMethod(invocation);

        verify(subscriptionManager).registerAttributeSubscription(eq(FROM_PARTICIPANT_ID),
                                                                  eq(toDiscoveryEntries),
                                                                  eq(invocation));
    }

    @Test
    public void testSubscribingToAttributeShouldFailIfDiscoveryEntrySetIsEmpty() throws NoSuchMethodException {
        final AttributeSubscriptionAdapter<String> listener = new AttributeSubscriptionAdapter<>();
        final OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        method = getSubscriptionMethod("subscribeToTestAttribute",
                                       AttributeSubscriptionListener.class,
                                       SubscriptionQos.class);
        final AttributeSubscribeInvocation invocation = new AttributeSubscribeInvocation(method,
                                                                                         new Object[]{ listener,
                                                                                                 subscriptionQos },
                                                                                         null,
                                                                                         proxy);
        toDiscoveryEntries.clear();
        try {
            handler.executeSubscriptionMethod(invocation);
        } catch (final JoynrIllegalStateException exception) {
            assertNotNull(exception);
            assertTrue(exception.getMessage().contains("at least one participant"));
            //noinspection unchecked
            verify(subscriptionManager, never()).registerAttributeSubscription(anyString(),
                                                                               any(Set.class),
                                                                               any(AttributeSubscribeInvocation.class));
        }
    }

    @Test
    public void testSubscriptionToBroadcastShouldSucceed() throws NoSuchMethodException {
        final TestBroadcastInterface.TestBroadcastAdapter listener = new TestBroadcastInterface.TestBroadcastAdapter();
        final OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        method = getBroadcastMethod("subscribeToTestBroadcast",
                                    TestBroadcastInterface.TestBroadcastListener.class,
                                    OnChangeSubscriptionQos.class,
                                    BroadcastFilterParameters.class);
        final BroadcastSubscribeInvocation invocation = new BroadcastSubscribeInvocation(method,
                                                                                         new Object[]{ listener,
                                                                                                 subscriptionQos,
                                                                                                 new BroadcastFilterParameters() },
                                                                                         null,
                                                                                         proxy);

        handler.executeSubscriptionMethod(invocation);
        verify(subscriptionManager).registerBroadcastSubscription(eq(FROM_PARTICIPANT_ID),
                                                                  eq(toDiscoveryEntries),
                                                                  eq(invocation));
    }

    @Test
    public void testSubscriptionToBroadcastShouldFailIfDiscoveryEntrySetIsEmpty() throws NoSuchMethodException {
        final TestBroadcastInterface.TestBroadcastAdapter listener = new TestBroadcastInterface.TestBroadcastAdapter();
        final OnChangeSubscriptionQos subscriptionQos = new OnChangeSubscriptionQos();
        method = getBroadcastMethod("subscribeToTestBroadcast",
                                    TestBroadcastInterface.TestBroadcastListener.class,
                                    OnChangeSubscriptionQos.class,
                                    BroadcastFilterParameters.class);
        final BroadcastSubscribeInvocation invocation = new BroadcastSubscribeInvocation(method,
                                                                                         new Object[]{ listener,
                                                                                                 subscriptionQos,
                                                                                                 new BroadcastFilterParameters() },
                                                                                         null,
                                                                                         proxy);
        toDiscoveryEntries.clear();
        try {
            handler.executeSubscriptionMethod(invocation);
        } catch (final JoynrIllegalStateException exception) {
            assertNotNull(exception);
            assertTrue(exception.getMessage().contains("at least one participant"));
            verify(subscriptionManager, never()).registerBroadcastSubscription(eq(FROM_PARTICIPANT_ID),
                                                                               eq(toDiscoveryEntries),
                                                                               eq(invocation));
        }
    }

    @Test
    public void testSubscriptionToMulticastShouldSucceed() throws NoSuchMethodException {
        final TestBroadcastInterface.TestBroadcastAdapter listener = new TestBroadcastInterface.TestBroadcastAdapter();
        final MulticastSubscriptionQos subscriptionQos = new MulticastSubscriptionQos();
        final String[] partitions = new String[]{ "partition1", "partition2", "partition3" };
        method = getBroadcastMethod("subscribeToTestMulticast",
                                    TestBroadcastInterface.TestBroadcastListener.class,
                                    MulticastSubscriptionQos.class,
                                    String[].class);
        final MulticastSubscribeInvocation invocation = new MulticastSubscribeInvocation(method,
                                                                                         new Object[]{ listener,
                                                                                                 subscriptionQos,
                                                                                                 partitions },
                                                                                         null,
                                                                                         proxy);

        handler.executeSubscriptionMethod(invocation);
        verify(subscriptionManager).registerMulticastSubscription(eq(FROM_PARTICIPANT_ID),
                                                                  eq(toDiscoveryEntries),
                                                                  eq(invocation));
    }

    @Test
    public void testSubscriptionToMulticastShouldFailIfDiscoveryEntrySetIsEmpty() throws NoSuchMethodException {
        final TestBroadcastInterface.TestBroadcastAdapter listener = new TestBroadcastInterface.TestBroadcastAdapter();
        final MulticastSubscriptionQos subscriptionQos = new MulticastSubscriptionQos();
        final String[] partitions = new String[]{ "partition1", "partition2", "partition3" };
        method = getBroadcastMethod("subscribeToTestMulticast",
                                    TestBroadcastInterface.TestBroadcastListener.class,
                                    MulticastSubscriptionQos.class,
                                    String[].class);
        final MulticastSubscribeInvocation invocation = new MulticastSubscribeInvocation(method,
                                                                                         new Object[]{ listener,
                                                                                                 subscriptionQos,
                                                                                                 partitions },
                                                                                         null,
                                                                                         proxy);

        toDiscoveryEntries.clear();
        try {
            handler.executeSubscriptionMethod(invocation);
        } catch (final JoynrIllegalStateException exception) {
            assertNotNull(exception);
            assertTrue(exception.getMessage().contains("at least one participant"));
            verify(subscriptionManager, never()).registerMulticastSubscription(eq(FROM_PARTICIPANT_ID),
                                                                               eq(toDiscoveryEntries),
                                                                               eq(invocation));
        }
    }

    private Method getSubscriptionMethod(final String methodName,
                                         final Class<?>... parameterTypes) throws NoSuchMethodException {
        return getMethod(TestSubscriptionInterface.class, methodName, parameterTypes);
    }

    private Method getBroadcastMethod(final String methodName,
                                      final Class<?>... parameterTypes) throws NoSuchMethodException {
        return getMethod(TestBroadcastInterface.class, methodName, parameterTypes);
    }
}
