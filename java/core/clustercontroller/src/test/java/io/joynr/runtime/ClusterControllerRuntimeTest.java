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
package io.joynr.runtime;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyBoolean;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Field;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.arbitration.ArbitratorFactory;
import io.joynr.arbitration.VersionCompatibilityChecker;
import io.joynr.capabilities.CapabilitiesRegistrar;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.capabilities.ParticipantIdStorage;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.messaging.routing.CcMessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.proxy.DiscoverySettingsStorage;
import io.joynr.proxy.StatelessAsyncCallbackDirectory;
import io.joynr.util.ObjectMapper;
import joynr.system.RoutingProvider;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.UdsAddress;

@RunWith(MockitoJUnitRunner.class)
public class ClusterControllerRuntimeTest {

    private final String systemServicesDomain = "testSystemServiceDomain";
    private Address dispatcherAddress = new InProcessAddress();
    private Address discoveryProviderAddress = new InProcessAddress();
    private long removeStaleDelayMs = 300000;

    private ObjectMapper objectMapper = new ObjectMapper();

    @Mock
    private Dispatcher dispatcherMock;
    @Mock
    private MessagingSkeletonFactory messagingSkeletonFactoryMock;
    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregatorMock;
    @Mock
    private RoutingTable routingTableMock;
    @Mock
    private StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectoryMock;
    @Mock
    private DiscoverySettingsStorage discoverySettingsStorageMock;
    @Mock
    private VersionCompatibilityChecker versionCompatibilityChecker;
    @Mock
    ParticipantIdStorage participantIdStorageMock;
    @Mock
    private CcMessageRouter messageRouterMock;
    @Mock
    CapabilitiesRegistrar capabilitiesRegistrarMock;
    @Mock
    private LocalCapabilitiesDirectory localCapabilitiesDirectoryMock;
    @Mock
    private RoutingProvider routingProviderMock;
    @Mock
    private ScheduledExecutorService removeStaleSchedulerMock;
    @Mock
    private ScheduledExecutorService arbitrationFactorySchedulerMock;
    @Mock
    private ShutdownNotifier shutdownNotifierMock;

    private ClusterControllerRuntime ccRuntime;

    @Before
    public void setUp() throws Exception {
        doNothing().when(routingTableMock).apply(any());

        Field arbitrationFactorySchedulerField = ArbitratorFactory.class.getDeclaredField("scheduler");
        arbitrationFactorySchedulerField.setAccessible(true);
        arbitrationFactorySchedulerField.set(ArbitratorFactory.class, arbitrationFactorySchedulerMock);

        Field shutdownNotifierField = ArbitratorFactory.class.getDeclaredField("shutdownNotifier");
        shutdownNotifierField.setAccessible(true);
        shutdownNotifierField.set(ArbitratorFactory.class, shutdownNotifierMock);
    }

    @Test
    public void removeStaleSchedulerUsesCorrectDelayMsValue() {
        String discoveryProviderParticipantIdMock = "discoveryProviderParticipantIdMock";
        doReturn(discoveryProviderParticipantIdMock).when(participantIdStorageMock)
                                                    .getProviderParticipantId(anyString(), anyString(), anyInt());

        doReturn(true).when(routingTableMock).put(eq(discoveryProviderParticipantIdMock),
                                                  any(Address.class),
                                                  anyBoolean(),
                                                  anyLong(),
                                                  anyBoolean());
        ccRuntime = new ClusterControllerRuntime(objectMapper,
                                                 null,
                                                 dispatcherMock,
                                                 messagingSkeletonFactoryMock,
                                                 localDiscoveryAggregatorMock,
                                                 routingTableMock,
                                                 statelessAsyncCallbackDirectoryMock,
                                                 discoverySettingsStorageMock,
                                                 versionCompatibilityChecker,
                                                 participantIdStorageMock,
                                                 systemServicesDomain,
                                                 dispatcherAddress,
                                                 discoveryProviderAddress,
                                                 messageRouterMock,
                                                 capabilitiesRegistrarMock,
                                                 localCapabilitiesDirectoryMock,
                                                 routingProviderMock,
                                                 removeStaleSchedulerMock,
                                                 removeStaleDelayMs);

        verify(removeStaleSchedulerMock, times(1)).schedule(any(Runnable.class),
                                                            eq(removeStaleDelayMs),
                                                            eq(TimeUnit.MILLISECONDS));

    }

    @Test(expected = JoynrRuntimeException.class)
    public void callCCRuntimeWithWrongAddress() {
        String discoveryProviderParticipantIdMock = "discoveryProviderParticipantIdMock";
        doReturn(discoveryProviderParticipantIdMock).when(participantIdStorageMock)
                                                    .getProviderParticipantId(anyString(), anyString(), anyInt());

        doReturn(false).when(routingTableMock).put(eq(discoveryProviderParticipantIdMock),
                                                   any(Address.class),
                                                   anyBoolean(),
                                                   anyLong(),
                                                   anyBoolean());

        Address discoveryProviderAddress = new UdsAddress();
        ccRuntime = new ClusterControllerRuntime(objectMapper,
                                                 null,
                                                 dispatcherMock,
                                                 messagingSkeletonFactoryMock,
                                                 localDiscoveryAggregatorMock,
                                                 routingTableMock,
                                                 statelessAsyncCallbackDirectoryMock,
                                                 discoverySettingsStorageMock,
                                                 versionCompatibilityChecker,
                                                 participantIdStorageMock,
                                                 systemServicesDomain,
                                                 dispatcherAddress,
                                                 discoveryProviderAddress,
                                                 messageRouterMock,
                                                 capabilitiesRegistrarMock,
                                                 localCapabilitiesDirectoryMock,
                                                 routingProviderMock,
                                                 removeStaleSchedulerMock,
                                                 removeStaleDelayMs);

    }
}
