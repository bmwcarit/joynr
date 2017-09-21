/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.atLeast;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.util.concurrent.DelayQueue;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.common.ExpiryDate;
import io.joynr.jeeintegration.messaging.JeeMessageRouter;
import io.joynr.messaging.FailureAction;
import io.joynr.messaging.IMessagingStub;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.SuccessAction;
import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.runtime.ShutdownNotifier;
import jersey.repackaged.com.google.common.collect.Sets;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;

@RunWith(MockitoJUnitRunner.class)
public class JeeMessageRouterIntegrationTest {
    private final long routingTableGracePeriodMs = 60000L;
    private final long routingTableCleanupIntervalMs = 60000L;

    @Mock
    private ScheduledExecutorService scheduler;

    @Mock
    private MessagingStubFactory messagingStubFactory;

    @Mock
    private MessagingSkeletonFactory messagingSkeletonFactory;

    @Mock
    private RoutingTable routingTable;

    @Mock
    private AddressManager addressManager;

    @Mock
    private MulticastReceiverRegistry multicastReceiverRegistry;

    @Mock
    DelayQueue<DelayableImmutableMessage> messageQueue;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Test(timeout=3000)
    public void testFailedTransmitDoesNotLeadToThreadStarvation() throws Exception {
        final int NUM_THREADS = 2;
        final int MESSAGE_LOAD = 10;
        final long SEND_MESSAGE_RETRY_INTERVAL_MS = 1;

        DelayQueue<DelayableImmutableMessage> messageQueue = new DelayQueue<>();
        ScheduledExecutorService scheduler = new ScheduledThreadPoolExecutor(NUM_THREADS);

        ImmutableMessage failingMessage = Mockito.mock(ImmutableMessage.class);
        when(failingMessage.isTtlAbsolute()).thenReturn(true);
        when(failingMessage.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        when(failingMessage.getRecipient()).thenReturn("to");

        IMessagingStub messagingStubMock = Mockito.mock(IMessagingStub.class);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                assertEquals(invocation.getArguments().length, 3);
                FailureAction failureAction = (FailureAction)invocation.getArguments()[2];
                failureAction.execute(new Exception("Some error"));
                return null;
            }
        }).when(messagingStubMock).transmit(eq(failingMessage),
                                            any(SuccessAction.class),
                                            any(FailureAction.class));

        Address address = new Address();
        when(routingTable.get("to")).thenReturn(address);
        when(addressManager.getAddresses(failingMessage)).thenReturn(Sets.newHashSet(address));
        when(messagingStubFactory.create(address)).thenReturn(messagingStubMock);

        JeeMessageRouter subject = new JeeMessageRouter(routingTable,
                                                        scheduler,
                                                        SEND_MESSAGE_RETRY_INTERVAL_MS,
                                                        NUM_THREADS,
                                                        routingTableGracePeriodMs,
                                                        routingTableCleanupIntervalMs,
                                                        messagingStubFactory,
                                                        messagingSkeletonFactory,
                                                        addressManager,
                                                        multicastReceiverRegistry,
                                                        null,
                                                        false,
                                                        messageQueue,
                                                        shutdownNotifier);

        for(int i=0; i < MESSAGE_LOAD; i++) {
            subject.route(failingMessage);
        }

        Thread.sleep(2000);
        verify(messagingStubMock, atLeast(MESSAGE_LOAD * 3)).transmit(eq(failingMessage),
                                                                      any(SuccessAction.class),
                                                                      any(FailureAction.class));

        ImmutableMessage anotherMessage = Mockito.mock(ImmutableMessage.class);
        when(anotherMessage.isTtlAbsolute()).thenReturn(true);
        when(anotherMessage.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(1000L).getValue());
        when(anotherMessage.getRecipient()).thenReturn("to");
        when(addressManager.getAddresses(anotherMessage)).thenReturn(Sets.newHashSet(address));

        final Semaphore semaphore = new Semaphore(0);
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                assertEquals(invocation.getArguments().length, 3);
                SuccessAction successAction = (SuccessAction)invocation.getArguments()[1];
                successAction.execute();
                semaphore.release();
                return null;
            }
        }).when(messagingStubMock).transmit(eq(anotherMessage),
                                            any(SuccessAction.class),
                                            any(FailureAction.class));

        subject.route(anotherMessage);
        assertTrue(semaphore.tryAcquire(100, TimeUnit.MILLISECONDS));
    }
}
