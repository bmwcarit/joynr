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
package io.joynr.messaging.routing;

import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.InvocationTargetException;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.runtime.ShutdownListener;
import io.joynr.runtime.ShutdownNotifier;
import joynr.ImmutableMessage;

@RunWith(MockitoJUnitRunner.class)
public class AbstractMessageRouterTest {

    private class DummyMessageRouter extends AbstractMessageRouter {
        public DummyMessageRouter(RoutingTable routingTable,
                                  ScheduledExecutorService scheduler,
                                  long sendMsgRetryIntervalMs,
                                  int maxParallelSends,
                                  long routingTableCleanupIntervalMs,
                                  MessagingStubFactory messagingStubFactory,
                                  MessagingSkeletonFactory messagingSkeletonFactory,
                                  AddressManager addressManager,
                                  MulticastReceiverRegistry multicastReceiverRegistry,
                                  MessageQueue messageQueue,
                                  ShutdownNotifier shutdownNotifier) {
            super(routingTable,
                  scheduler,
                  sendMsgRetryIntervalMs,
                  maxParallelSends,
                  routingTableCleanupIntervalMs,
                  messagingStubFactory,
                  messagingSkeletonFactory,
                  addressManager,
                  multicastReceiverRegistry,
                  messageQueue,
                  shutdownNotifier);
        }

        @Override
        public void setToKnown(String participantId) {
            // Do nothing
        }

        @Override
        public void routeIn(ImmutableMessage message) {
        }

        @Override
        public void routeOut(ImmutableMessage message) {
        }
    }

    @Mock
    private RoutingTable mockRoutingTable;
    @Mock
    private MessagingStubFactory mockMessagingStubFactory;
    @Mock
    private MessagingSkeletonFactory mockMessagingSkeletonFactory;
    @Mock
    private AddressManager mockAddressManager;
    @Mock
    private MulticastReceiverRegistry mockMultiCastReceiverRegistry;
    @Mock
    private MessageQueue mockMessageQueue;
    @Mock
    private ShutdownNotifier mockShutdownNotifier;
    @Mock
    private ShutdownListener mockShutdownListener;
    @Mock
    private Object mockObject;

    private final ScheduledExecutorService scheduler = Mockito.spy(new ScheduledThreadPoolExecutor(42));

    private final long sendMsgRetryIntervalMs = 100;
    private final int maxParallelSends = 1;
    private final long routingTableCleanupIntervalMs = 1000;

    @SuppressWarnings("unused")
    private DummyMessageRouter subject;

    @Before
    public void setup() {
        subject = new DummyMessageRouter(mockRoutingTable,
                                         scheduler,
                                         sendMsgRetryIntervalMs,
                                         maxParallelSends,
                                         routingTableCleanupIntervalMs,
                                         mockMessagingStubFactory,
                                         mockMessagingSkeletonFactory,
                                         mockAddressManager,
                                         mockMultiCastReceiverRegistry,
                                         mockMessageQueue,
                                         mockShutdownNotifier);
    }

    @Test
    public void cleanupJobRemovesPurgesExpiredRoutingEntries() throws IllegalAccessException, IllegalArgumentException,
                                                               InvocationTargetException {
        // Capture Runnable when cleanup job is invoked
        ArgumentCaptor<Runnable> runnableCaptor = ArgumentCaptor.forClass(Runnable.class);
        verify(scheduler, times(1)).scheduleWithFixedDelay(runnableCaptor.capture(),
                                                           eq(routingTableCleanupIntervalMs),
                                                           eq(routingTableCleanupIntervalMs),
                                                           eq(TimeUnit.MILLISECONDS));

        runnableCaptor.getValue().run();

        verify(mockRoutingTable).purge();
    }

}
