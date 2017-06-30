/**
 *
 */
package test.io.joynr.jeeintegration.messaging;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

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

import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import io.joynr.common.ExpiryDate;
import io.joynr.jeeintegration.messaging.JeeMessageRouter;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.routing.AddressManager;
import io.joynr.messaging.routing.DelayableImmutableMessage;
import io.joynr.messaging.routing.BoundedDelayQueue;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import io.joynr.messaging.routing.RoutingTable;
import joynr.ImmutableMessage;
import joynr.system.RoutingTypes.Address;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

/**
 * Unit tests for the {@link io.joynr.jeeintegration.messaging.JeeMessageRouter}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeMessageRouterTest {
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
    BoundedDelayQueue<DelayableImmutableMessage> messageQueue;

    @Test
    public void testScheduleMessage() throws InterruptedException {
        Address address = new Address();
        ImmutableMessage message = Mockito.mock(ImmutableMessage.class);
        when(message.isTtlAbsolute()).thenReturn(true);
        when(message.getTtlMs()).thenReturn(ExpiryDate.fromRelativeTtl(60000L).getValue());
        when(message.getRecipient()).thenReturn("to");
        when(routingTable.get("to")).thenReturn(address);

        JeeMessageRouter subject = new JeeMessageRouter(routingTable,
                                                        scheduler,
                                                        1000L,
                                                        10,
                                                        routingTableGracePeriodMs,
                                                        routingTableCleanupIntervalMs,
                                                        messagingStubFactory,
                                                        messagingSkeletonFactory,
                                                        addressManager,
                                                        multicastReceiverRegistry,
                                                        null,
                                                        false,
                                                        messageQueue);

        subject.route(message);

        ArgumentCaptor<DelayableImmutableMessage> passedDelaybleMessage = ArgumentCaptor.forClass(DelayableImmutableMessage.class);
        verify(messageQueue).putBounded(passedDelaybleMessage.capture());
        assertEquals(message, passedDelaybleMessage.getValue().getMessage());
        assertTrue(passedDelaybleMessage.getValue().getDelay(TimeUnit.MILLISECONDS) <= 0);

    }
}
