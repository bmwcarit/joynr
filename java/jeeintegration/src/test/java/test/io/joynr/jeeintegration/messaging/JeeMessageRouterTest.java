/**
 *
 */
package test.io.joynr.jeeintegration.messaging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.common.ExpiryDate;
import io.joynr.jeeintegration.messaging.JeeMessageRouter;
import io.joynr.messaging.routing.MessagingStubFactory;
import io.joynr.messaging.routing.RoutingTable;
import joynr.JoynrMessage;
import joynr.system.RoutingTypes.Address;

/**
 * Unit tests for the {@link io.joynr.jeeintegration.messaging.JeeMessageRouter}.
 *
 * @author clive.jevons commissioned by MaibornWolff
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeMessageRouterTest {

    @Test
    public void testScheduleMessage() {
        Address address = new Address();
        JoynrMessage message = new JoynrMessage();
        message.setTo("to");
        ScheduledExecutorService scheduler = mock(ScheduledExecutorService.class);
        MessagingStubFactory messagingStubFactory = mock(MessagingStubFactory.class);
        RoutingTable routingTable = mock(RoutingTable.class);
        when(routingTable.get("to")).thenReturn(address);
        JeeMessageRouter subject = new JeeMessageRouter(routingTable, scheduler, 1000L, messagingStubFactory);

        message.setExpirationDate(ExpiryDate.fromRelativeTtl(60000L));
        subject.route(message);

        verify(scheduler).schedule((Runnable) Mockito.any(), Mockito.eq(0L), Mockito.eq(TimeUnit.MILLISECONDS));
    }

    @Test
    public void testShutdown() throws InterruptedException {
        ScheduledExecutorService scheduler = mock(ScheduledExecutorService.class);
        MessagingStubFactory messagingStubFactory = mock(MessagingStubFactory.class);
        RoutingTable routingTable = mock(RoutingTable.class);
        JeeMessageRouter subject = new JeeMessageRouter(routingTable, scheduler, 1000L, messagingStubFactory);

        subject.shutdown();

        verify(scheduler).shutdown();
    }

}
