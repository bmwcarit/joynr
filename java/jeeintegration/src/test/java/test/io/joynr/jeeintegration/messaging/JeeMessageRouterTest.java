/**
 *
 */
package test.io.joynr.jeeintegration.messaging;

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
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.MockChannelAddressFactory;
import io.joynr.messaging.routing.MulticastReceiverRegistry;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.runtime.ReplyToAddressProvider;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import com.google.common.collect.Sets;

/**
 * Unit tests for the {@link io.joynr.jeeintegration.messaging.JeeMessageRouter}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeMessageRouterTest {

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

    @Test
    public void testScheduleMessage() {
        Address address = new Address();
        JoynrMessage message = new JoynrMessage();
        message.setTo("to");
        when(routingTable.get("to")).thenReturn(address);

        @SuppressWarnings("unchecked")
        JeeMessageRouter subject = new JeeMessageRouter(new ReplyToAddressProvider(Sets.newHashSet(new MockChannelAddressFactory())),
                                                        routingTable,
                                                        scheduler,
                                                        1000L,
                                                        messagingStubFactory,
                                                        messagingSkeletonFactory,
                                                        addressManager,
                                                        multicastReceiverRegistry,
                                                        null,
                                                        false);

        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000L));
        subject.route(message);

        verify(scheduler).schedule((Runnable) Mockito.any(), Mockito.eq(0L), Mockito.eq(TimeUnit.MILLISECONDS));
    }

    @Test
    public void testShutdown() throws InterruptedException {
        @SuppressWarnings("unchecked")
        JeeMessageRouter subject = new JeeMessageRouter(new ReplyToAddressProvider(Sets.newHashSet(new MockChannelAddressFactory())),
                                                        routingTable,
                                                        scheduler,
                                                        1000L,
                                                        messagingStubFactory,
                                                        messagingSkeletonFactory,
                                                        addressManager,
                                                        multicastReceiverRegistry,
                                                        null,
                                                        false);

        subject.shutdown();

        verify(scheduler).shutdown();
    }

}
