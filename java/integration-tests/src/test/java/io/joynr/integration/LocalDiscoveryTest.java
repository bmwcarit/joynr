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
package io.joynr.integration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;
import java.util.stream.Collectors;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.AbstractModule;
import com.google.inject.Inject;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.name.Named;
import com.google.inject.name.Names;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategyFunction;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.CapabilitiesProvisioning;
import io.joynr.capabilities.CapabilityUtils;
import io.joynr.capabilities.DiscoveryEntryStore;
import io.joynr.capabilities.ExpiredDiscoveryEntryCacheCleaner;
import io.joynr.capabilities.GlobalCapabilitiesDirectoryClient;
import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.capabilities.LocalCapabilitiesDirectoryImpl;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ConnectorFactory;
import io.joynr.proxy.Future;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyInvocationHandler;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.ProxyInvocationHandlerImpl;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.JoynrBaseModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.runtime.SystemServicesSettings;
import io.joynr.util.VersionUtil;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.MqttAddress;
import joynr.test.JoynrTestLoggingRule;
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;

class ProxyInvocationHandlerFactoryImpl implements ProxyInvocationHandlerFactory {

    private ConnectorFactory connectorFactory;
    private ConnectorFactory connectorFactoryMock;
    private MessageRouter messageRouter;
    private ShutdownNotifier shutdownNotifier;

    @Inject
    public ProxyInvocationHandlerFactoryImpl(ConnectorFactory connectorFactory,
                                             @Named("connectorFactoryMock") JoynrMessagingConnectorFactory connectorFactoryMock,
                                             MessageRouter messageRouter,
                                             @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                                             ShutdownNotifier shutdownNotifier) {
        super();
        this.messageRouter = messageRouter;
        this.connectorFactory = connectorFactory;
        this.connectorFactoryMock = new ConnectorFactory(connectorFactoryMock, messageRouter, dispatcherAddress);
        this.shutdownNotifier = shutdownNotifier;
    }

    @Override
    public ProxyInvocationHandler create(Set<String> domains,
                                         String interfaceName,
                                         String proxyParticipantId,
                                         DiscoveryQos discoveryQos,
                                         MessagingQos messagingQos,
                                         ShutdownNotifier shutdownNotifier) {
        if (domains.contains("io.joynr.system")) {
            return new ProxyInvocationHandlerImpl(domains,
                                                  interfaceName,
                                                  proxyParticipantId,
                                                  discoveryQos,
                                                  messagingQos,
                                                  connectorFactory,
                                                  messageRouter,
                                                  shutdownNotifier);
        }
        return new ProxyInvocationHandlerImpl(domains,
                                              interfaceName,
                                              proxyParticipantId,
                                              discoveryQos,
                                              messagingQos,
                                              connectorFactoryMock,
                                              messageRouter,
                                              shutdownNotifier);
    }

}

/**
 * Test class to check whether the DiscoveryEntryWithMetaInfo from local or global lookup is correctly made available
 * to the JoynrMessagingConnector in libjoynr.
 */
@RunWith(MockitoJUnitRunner.class)
public class LocalDiscoveryTest {
    private static final Logger logger = LoggerFactory.getLogger(LocalDiscoveryTest.class);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    private JoynrRuntime runtime;

    @Mock
    private DiscoveryEntryStore localDiscoveryEntryStoreMock;
    @Mock
    private DiscoveryEntryStore globalDiscoveryEntryCacheMock;
    @Mock
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClientMock;
    @Mock
    private GlobalAddressProvider globalAddressProviderMock;
    @Mock
    private CapabilitiesProvisioning capabilitiesProvisioningMock;
    @Mock
    private MessageRouter messageRouterMock;
    @Mock
    private ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleanerMock;
    @Mock
    private ScheduledExecutorService capabilitiesFreshnessUpdateExecutorMock;
    @Mock
    private JoynrMessagingConnectorFactory joynrMessagingConnectorFactoryMock;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Captor
    private ArgumentCaptor<Set<DiscoveryEntryWithMetaInfo>> discoveryEntryWithMetaInfoArgumentCaptor;

    private final long defaultDiscoveryRetryIntervalMs = 2000L;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        Mockito.doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        // use default freshnessUpdateIntervalMs: 3600000ms (1h)
        final LocalCapabilitiesDirectoryImpl localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioningMock,
                                                                                                             globalAddressProviderMock,
                                                                                                             localDiscoveryEntryStoreMock,
                                                                                                             globalDiscoveryEntryCacheMock,
                                                                                                             messageRouterMock,
                                                                                                             globalCapabilitiesDirectoryClientMock,
                                                                                                             expiredDiscoveryEntryCacheCleanerMock,
                                                                                                             3600000,
                                                                                                             capabilitiesFreshnessUpdateExecutorMock,
                                                                                                             defaultDiscoveryRetryIntervalMs,
                                                                                                             shutdownNotifier);

        Module testModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule(),
                                                                                  new AbstractModule() {
                                                                                      @Override
                                                                                      protected void configure() {
                                                                                          bind(JoynrMessagingConnectorFactory.class).annotatedWith(Names.named("connectorFactoryMock"))
                                                                                                                                    .toInstance(joynrMessagingConnectorFactoryMock);
                                                                                          bind(LocalCapabilitiesDirectory.class).toInstance(localCapabilitiesDirectory);
                                                                                          bind(LocalCapabilitiesDirectoryImpl.class).toInstance(localCapabilitiesDirectory);
                                                                                          bind(ProxyInvocationHandlerFactory.class).to(ProxyInvocationHandlerFactoryImpl.class);
                                                                                      }
                                                                                  });
        Properties joynrProperties = new Properties();
        Injector injector = new JoynrInjectorFactory(new JoynrBaseModule(joynrProperties, testModule)).getInjector();

        runtime = injector.getInstance(JoynrRuntime.class);
    }

    @Test
    public void testLocalDiscoveryEntries() {
        String testDomain = "testDomain";
        String interfaceName = testProxy.INTERFACE_NAME;
        Collection<DiscoveryEntry> discoveryEntries = new HashSet<>();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                           testDomain,
                                                           interfaceName,
                                                           "participantId",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           System.currentTimeMillis() + 100000,
                                                           "publicKeyId");
        discoveryEntries.add(discoveryEntry);

        Mockito.doReturn(discoveryEntries).when(localDiscoveryEntryStoreMock).lookup(any(String[].class),
                                                                                     eq(interfaceName));

        ProxyBuilder<testProxy> proxyBuilder = runtime.getProxyBuilder(testDomain, testProxy.class);
        final Future<Void> future = new Future<Void>();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<testProxy>() {
            @Override
            public void onProxyCreationFinished(testProxy result) {
                future.onSuccess(null);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                future.onFailure(error);
            }
        });

        try {
            future.get(5000);
            verify(joynrMessagingConnectorFactoryMock).create(anyString(),
                                                              discoveryEntryWithMetaInfoArgumentCaptor.capture(),
                                                              any(MessagingQos.class));

            assertDiscoveryEntryEqualsCaptured(discoveryEntry);

        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    @Test
    public void testCachedGlobalDiscoveryEntries() {
        String testDomain = "testDomain";
        String interfaceName = testProxy.INTERFACE_NAME;
        Collection<DiscoveryEntry> discoveryEntries = new HashSet<>();
        discoveryEntries.add(new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                testDomain,
                                                interfaceName,
                                                "participantId",
                                                new ProviderQos(),
                                                System.currentTimeMillis(),
                                                System.currentTimeMillis() + 100000,
                                                "publicKeyId"));
        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(false,
                                                                                                                              discoveryEntries);

        Mockito.doReturn(discoveryEntries).when(globalDiscoveryEntryCacheMock).lookup(any(String[].class),
                                                                                      eq(interfaceName),
                                                                                      anyLong());

        ProxyBuilder<testProxy> proxyBuilder = runtime.getProxyBuilder(testDomain, testProxy.class);
        final Future<Void> future = new Future<Void>();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<testProxy>() {
            @Override
            public void onProxyCreationFinished(testProxy result) {
                future.onSuccess(null);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                future.onFailure(error);
            }
        });

        try {
            future.get(5000);
            verify(joynrMessagingConnectorFactoryMock).create(anyString(),
                                                              eq(discoveryEntriesWithMetaInfo),
                                                              any(MessagingQos.class));
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testRemoteGlobalDiscoveryEntries() {
        String testDomain = "testDomain";
        String interfaceName = testProxy.INTERFACE_NAME;
        final Collection<DiscoveryEntry> discoveryEntries = new HashSet<>();
        final List<GlobalDiscoveryEntry> globalDiscoveryEntries = new ArrayList();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                           testDomain,
                                                           interfaceName,
                                                           "participantId",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           System.currentTimeMillis() + 100000,
                                                           "publicKeyId");
        discoveryEntries.add(discoveryEntry);
        globalDiscoveryEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry,
                                                                                       new MqttAddress()));

        Mockito.doReturn(new HashSet<DiscoveryEntry>()).when(globalDiscoveryEntryCacheMock).lookup(any(String[].class),
                                                                                                   eq(interfaceName),
                                                                                                   anyLong());
        Mockito.doAnswer(new Answer<Object>() {

            @SuppressWarnings("rawtypes")
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[0] instanceof Callback);
                ((Callback) arguments[0]).resolve((Object) globalDiscoveryEntries);
                return null;
            }
        })
               .when(globalCapabilitiesDirectoryClientMock)
               .lookup(any(Callback.class), any(String[].class), eq(interfaceName), anyLong());

        ProxyBuilder<testProxy> proxyBuilder = runtime.getProxyBuilder(testDomain, testProxy.class);
        final Future<Void> future = new Future<Void>();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<testProxy>() {
            @Override
            public void onProxyCreationFinished(testProxy result) {
                future.onSuccess(null);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                future.onFailure(error);
            }
        });

        try {
            future.get(5000);

            verify(joynrMessagingConnectorFactoryMock).create(anyString(),
                                                              discoveryEntryWithMetaInfoArgumentCaptor.capture(),
                                                              any(MessagingQos.class));

            assertDiscoveryEntryEqualsCaptured(discoveryEntry);
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testMixedLocalCachedRemoteDiscoveryEntries() {
        String testDomain = "testDomain";
        String remoteDomain = "remoteDomain";
        Set<String> testDomains = new HashSet<>();
        testDomains.add(testDomain);
        testDomains.add(remoteDomain);
        String interfaceName = testProxy.INTERFACE_NAME;
        final Collection<DiscoveryEntry> localDiscoveryEntries = new HashSet<>();
        final Collection<DiscoveryEntry> cachedDiscoveryEntries = new HashSet<>();
        final List<GlobalDiscoveryEntry> remoteDiscoveryEntries = new ArrayList();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                           testDomain,
                                                           interfaceName,
                                                           "participantIdLocal",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           System.currentTimeMillis() + 100000,
                                                           "publicKeyId");
        DiscoveryEntry cachedDiscoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                                 testDomain,
                                                                 interfaceName,
                                                                 "participantIdCached",
                                                                 new ProviderQos(),
                                                                 System.currentTimeMillis(),
                                                                 System.currentTimeMillis() + 100000,
                                                                 "publicKeyId");
        GlobalDiscoveryEntry remoteDiscoveryEntry = CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                                                                                           remoteDomain,
                                                                                                                           interfaceName,
                                                                                                                           "participantIdRemote",
                                                                                                                           new ProviderQos(),
                                                                                                                           System.currentTimeMillis(),
                                                                                                                           System.currentTimeMillis()
                                                                                                                                   + 100000,
                                                                                                                           "publicKeyId"),
                                                                                                        new MqttAddress());
        localDiscoveryEntries.add(discoveryEntry);
        cachedDiscoveryEntries.add(cachedDiscoveryEntry);
        remoteDiscoveryEntries.add(remoteDiscoveryEntry);
        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(true,
                                                                                                                              localDiscoveryEntries);
        discoveryEntriesWithMetaInfo.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                             cachedDiscoveryEntry));
        discoveryEntriesWithMetaInfo.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                             remoteDiscoveryEntry));

        Mockito.doReturn(localDiscoveryEntries).when(localDiscoveryEntryStoreMock).lookup(any(String[].class),
                                                                                          eq(interfaceName));
        Mockito.doReturn(cachedDiscoveryEntries).when(globalDiscoveryEntryCacheMock).lookup(any(String[].class),
                                                                                            eq(interfaceName),
                                                                                            anyLong());
        Mockito.doAnswer(new Answer<Object>() {

            @SuppressWarnings("rawtypes")
            @Override
            public List<GlobalDiscoveryEntry> answer(InvocationOnMock invocation) throws Throwable {
                Object[] arguments = invocation.getArguments();
                assert (arguments[0] instanceof Callback);
                ((Callback) arguments[0]).resolve((Object) remoteDiscoveryEntries);
                return null;
            }
        })
               .when(globalCapabilitiesDirectoryClientMock)
               .lookup(any(Callback.class), any(String[].class), eq(interfaceName), anyLong());

        ProxyBuilder<testProxy> proxyBuilder = runtime.getProxyBuilder(testDomains, testProxy.class);
        final Future<Void> future = new Future<Void>();
        ArbitrationStrategyFunction arbitrationStrategyFunction = new ArbitrationStrategyFunction() {
            @Override
            public Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                          Collection<DiscoveryEntryWithMetaInfo> capabilities) {
                return capabilities.stream().collect(Collectors.toSet());
            }
        };
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     arbitrationStrategyFunction,
                                                     0,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL);

        proxyBuilder.setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<testProxy>() {
            @Override
            public void onProxyCreationFinished(testProxy result) {
                future.onSuccess(null);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                future.onFailure(error);
            }
        });

        try {
            future.get(5000);
            verify(joynrMessagingConnectorFactoryMock).create(anyString(),
                                                              eq(discoveryEntriesWithMetaInfo),
                                                              any(MessagingQos.class));
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    private void assertDiscoveryEntryEqualsCaptured(DiscoveryEntry discoveryEntry) {
        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesCaptured = discoveryEntryWithMetaInfoArgumentCaptor.getAllValues()
                                                                                                           .get(0);
        assertTrue(discoveryEntriesCaptured.size() == 1);
        DiscoveryEntryWithMetaInfo discoveryEntryCaptured = discoveryEntriesCaptured.iterator().next();
        assertEquals(discoveryEntry.getDomain(), discoveryEntryCaptured.getDomain());
        assertEquals(discoveryEntry.getInterfaceName(), discoveryEntryCaptured.getInterfaceName());
        assertEquals(discoveryEntry.getParticipantId(), discoveryEntryCaptured.getParticipantId());
    }
}
