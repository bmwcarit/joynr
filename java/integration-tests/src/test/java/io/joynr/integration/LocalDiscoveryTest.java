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

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyLong;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Properties;
import java.util.Set;
import java.util.concurrent.ScheduledExecutorService;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatchers;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.junit.MockitoJUnitRunner;
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

import io.joynr.accesscontrol.AccessController;
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
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.GarbageCollectionHandler;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.messaging.routing.RoutingTable;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.proxy.Callback;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.ConnectorFactory;
import io.joynr.proxy.DefaultStatelessAsyncIdCalculatorImpl;
import io.joynr.proxy.Future;
import io.joynr.proxy.JoynrMessagingConnectorFactory;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.proxy.ProxyInvocationHandler;
import io.joynr.proxy.ProxyInvocationHandlerFactory;
import io.joynr.proxy.ProxyInvocationHandlerImpl;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.proxy.StatelessAsyncIdCalculator;
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
import joynr.types.DiscoveryError;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;

class ProxyInvocationHandlerFactoryImpl implements ProxyInvocationHandlerFactory {

    private ConnectorFactory connectorFactory;
    private ConnectorFactory connectorFactoryMock;
    private MessageRouter messageRouter;
    private GarbageCollectionHandler gcHandler;
    private StatelessAsyncIdCalculator statelessAsyncIdCalculator;

    @Inject
    public ProxyInvocationHandlerFactoryImpl(ConnectorFactory connectorFactory,
                                             @Named("connectorFactoryMock") JoynrMessagingConnectorFactory connectorFactoryMock,
                                             MessageRouter messageRouter,
                                             GarbageCollectionHandler gcHandler,
                                             @Named(SystemServicesSettings.PROPERTY_DISPATCHER_ADDRESS) Address dispatcherAddress,
                                             ShutdownNotifier shutdownNotifier,
                                             StatelessAsyncIdCalculator statelessAsyncIdCalculator) {
        super();
        this.messageRouter = messageRouter;
        this.gcHandler = gcHandler;
        this.connectorFactory = connectorFactory;
        this.connectorFactoryMock = new ConnectorFactory(connectorFactoryMock, messageRouter, dispatcherAddress);
        this.statelessAsyncIdCalculator = statelessAsyncIdCalculator;
    }

    @Override
    public ProxyInvocationHandler create(Set<String> domains,
                                         String interfaceName,
                                         String proxyParticipantId,
                                         DiscoveryQos discoveryQos,
                                         MessagingQos messagingQos,
                                         ShutdownNotifier shutdownNotifier,
                                         Optional<StatelessAsyncCallback> statelessAsyncCallback) {
        if (domains.contains("io.joynr.system")) {
            return new ProxyInvocationHandlerImpl(domains,
                                                  interfaceName,
                                                  proxyParticipantId,
                                                  discoveryQos,
                                                  messagingQos,
                                                  statelessAsyncCallback,
                                                  connectorFactory,
                                                  messageRouter,
                                                  gcHandler,
                                                  shutdownNotifier,
                                                  statelessAsyncIdCalculator);
        }
        return new ProxyInvocationHandlerImpl(domains,
                                              interfaceName,
                                              proxyParticipantId,
                                              discoveryQos,
                                              messagingQos,
                                              statelessAsyncCallback,
                                              connectorFactoryMock,
                                              messageRouter,
                                              gcHandler,
                                              shutdownNotifier,
                                              statelessAsyncIdCalculator);
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

    private Injector injector;

    private JoynrRuntime runtime;

    @Mock
    private DiscoveryEntryStore<DiscoveryEntry> localDiscoveryEntryStoreMock;
    @Mock
    private DiscoveryEntryStore<GlobalDiscoveryEntry> globalDiscoveryEntryCacheMock;
    @Mock
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesDirectoryClientMock;
    @Mock
    private GlobalAddressProvider globalAddressProviderMock;
    @Mock
    private CapabilitiesProvisioning capabilitiesProvisioningMock;
    @Mock
    private RoutingTable routingTableMock;
    @Mock
    private ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleanerMock;
    @Mock
    private ScheduledExecutorService capabilitiesFreshnessUpdateExecutorMock;
    @Mock
    private JoynrMessagingConnectorFactory joynrMessagingConnectorFactoryMock;
    @Mock
    private ShutdownNotifier shutdownNotifier;
    @Mock
    private AccessController accessController;

    @Captor
    private ArgumentCaptor<Set<DiscoveryEntryWithMetaInfo>> discoveryEntryWithMetaInfoArgumentCaptor;
    @Captor
    private ArgumentCaptor<Set<DiscoveryEntryWithMetaInfo>> discoveryEntryWithMetaInfoArgumentCaptorForCachedEntry;

    private final String[] defaultGbids = { "testgbid1", "testgbid2" };
    private MqttAddress globalAddress;

    private boolean enableAccessControl = false;

    @Before
    public void setUp() {
        MockitoAnnotations.openMocks(this);
        // use default freshnessUpdateIntervalMs: 3600000ms (1h)
        final long defaultExpiryTime = 3628800000l;
        final LocalCapabilitiesDirectoryImpl localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioningMock,
                                                                                                             globalAddressProviderMock,
                                                                                                             localDiscoveryEntryStoreMock,
                                                                                                             globalDiscoveryEntryCacheMock,
                                                                                                             routingTableMock,
                                                                                                             globalCapabilitiesDirectoryClientMock,
                                                                                                             expiredDiscoveryEntryCacheCleanerMock,
                                                                                                             3600000,
                                                                                                             capabilitiesFreshnessUpdateExecutorMock,
                                                                                                             shutdownNotifier,
                                                                                                             defaultGbids,
                                                                                                             defaultExpiryTime,
                                                                                                             accessController,
                                                                                                             enableAccessControl);

        Module testModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule(),
                                                                                  new AbstractModule() {
                                                                                      @Override
                                                                                      protected void configure() {
                                                                                          bind(JoynrMessagingConnectorFactory.class).annotatedWith(Names.named("connectorFactoryMock"))
                                                                                                                                    .toInstance(joynrMessagingConnectorFactoryMock);
                                                                                          bind(LocalCapabilitiesDirectory.class).toInstance(localCapabilitiesDirectory);
                                                                                          bind(LocalCapabilitiesDirectoryImpl.class).toInstance(localCapabilitiesDirectory);
                                                                                          bind(ProxyInvocationHandlerFactory.class).to(ProxyInvocationHandlerFactoryImpl.class);
                                                                                          bind(StatelessAsyncIdCalculator.class).to(DefaultStatelessAsyncIdCalculatorImpl.class);
                                                                                          bind(String[].class).annotatedWith(Names.named(MessagingPropertyKeys.GBID_ARRAY))
                                                                                                              .toInstance(defaultGbids);
                                                                                      }
                                                                                  });
        Properties joynrProperties = new Properties();
        injector = new JoynrInjectorFactory(new JoynrBaseModule(joynrProperties, testModule)).getInjector();

        globalAddress = new MqttAddress(defaultGbids[0], "testOwnTopic");
    }

    @Test
    public void testLocalDiscoveryEntries() {
        String testDomain = "testDomain";
        String interfaceName = testProxy.INTERFACE_NAME;
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
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
        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(true,
                                                                                                                              discoveryEntries);

        doReturn(discoveryEntries).when(localDiscoveryEntryStoreMock).lookup(any(String[].class), eq(interfaceName));

        runtime = injector.getInstance(JoynrRuntime.class);
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
                                                              eq(discoveryEntriesWithMetaInfo),
                                                              any(MessagingQos.class),
                                                              eq(null));
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    private Answer<Future<List<GlobalDiscoveryEntry>>> createLookupAnswer(final List<GlobalDiscoveryEntry> caps) {
        return new Answer<Future<List<GlobalDiscoveryEntry>>>() {

            @Override
            public Future<List<GlobalDiscoveryEntry>> answer(InvocationOnMock invocation) throws Throwable {
                Future<List<GlobalDiscoveryEntry>> result = new Future<List<GlobalDiscoveryEntry>>();
                result.onSuccess(caps);
                @SuppressWarnings("unchecked")
                Callback<List<GlobalDiscoveryEntry>> callback = (Callback<List<GlobalDiscoveryEntry>>) invocation.getArguments()[0];
                callback.onSuccess(caps);
                return result;
            }
        };
    }

    private void verifyGlobalLookup(String interfaceName, String[] testDomains) {
        verify(globalCapabilitiesDirectoryClientMock).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                             eq(testDomains),
                                                             eq(interfaceName),
                                                             anyLong(),
                                                             any(String[].class));
    }

    @Test
    public void testCachedGlobalDiscoveryEntries() {
        String testDomain = "testDomain";
        String interfaceName = testProxy.INTERFACE_NAME;
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        Collection<DiscoveryEntry> discoveryEntries = new HashSet<>();
        GlobalDiscoveryEntry cachedEntry = new GlobalDiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                                    testDomain,
                                                                    interfaceName,
                                                                    "participantId",
                                                                    new ProviderQos(),
                                                                    System.currentTimeMillis(),
                                                                    System.currentTimeMillis() + 100000,
                                                                    "publicKeyId",
                                                                    CapabilityUtils.serializeAddress(globalAddress));
        discoveryEntries.add(cachedEntry);
        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(false,
                                                                                                                              discoveryEntries);

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        doReturn(Arrays.asList(cachedEntry)).when(globalDiscoveryEntryCacheMock)
                                            .lookup(eq(new String[]{ testDomain }),
                                                    eq(cachedEntry.getInterfaceName()),
                                                    eq(discoveryQos.getCacheMaxAgeMs()));

        runtime = injector.getInstance(JoynrRuntime.class);
        ProxyBuilder<testProxy> proxyBuilder = runtime.getProxyBuilder(testDomain, testProxy.class);
        final Future<Void> future = new Future<Void>();

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
                                                              any(MessagingQos.class),
                                                              eq(null));
            verify(globalCapabilitiesDirectoryClientMock,
                   times(0)).lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                    Mockito.<String[]> any(),
                                    anyString(),
                                    anyLong(),
                                    any(String[].class));
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    @Test
    public void testRemoteGlobalDiscoveryEntries() {
        String testDomain = "testDomain";
        String[] testDomains = { testDomain };
        String interfaceName = testProxy.INTERFACE_NAME;
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        final List<GlobalDiscoveryEntry> globalDiscoveryEntries = new ArrayList<>();
        final Set<DiscoveryEntryWithMetaInfo> discoveryEntriesWithMetaInfo = new HashSet<>();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                           testDomain,
                                                           interfaceName,
                                                           "participantId",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           System.currentTimeMillis() + 100000,
                                                           "publicKeyId");
        discoveryEntriesWithMetaInfo.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false, discoveryEntry));
        globalDiscoveryEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(discoveryEntry, globalAddress));

        doReturn(new HashSet<DiscoveryEntry>()).when(globalDiscoveryEntryCacheMock)
                                               .lookup(any(String[].class), eq(interfaceName), anyLong());
        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock).lookup(anyString(), anyLong());
        doAnswer(createLookupAnswer(globalDiscoveryEntries)).when(globalCapabilitiesDirectoryClientMock)
                                                            .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                    any(String[].class),
                                                                    eq(interfaceName),
                                                                    anyLong(),
                                                                    any(String[].class));

        runtime = injector.getInstance(JoynrRuntime.class);
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
                                                              any(MessagingQos.class),
                                                              eq(null));

            verifyGlobalLookup(interfaceName, testDomains);
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

    @Test
    public void testMixedLocalCachedRemoteDiscoveryEntries() throws InterruptedException {
        String localAndCacheDomain = "localAndCacheDomain";
        String remoteDomain = "remoteDomain";
        Set<String> testDomains = new HashSet<>();
        testDomains.add(localAndCacheDomain);
        testDomains.add(remoteDomain);
        doReturn(true).when(localDiscoveryEntryStoreMock).hasDiscoveryEntry(any(DiscoveryEntry.class));
        String interfaceName = testProxy.INTERFACE_NAME;
        ArbitrationStrategyFunction arbitrationStrategyFunction = new ArbitrationStrategyFunction() {
            @Override
            public Set<DiscoveryEntryWithMetaInfo> select(Map<String, String> parameters,
                                                          Collection<DiscoveryEntryWithMetaInfo> capabilities) {
                return new HashSet<DiscoveryEntryWithMetaInfo>(capabilities);
            }
        };
        DiscoveryQos discoveryQos = new DiscoveryQos(300009998,
                                                     arbitrationStrategyFunction,
                                                     0,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL);

        final Collection<DiscoveryEntry> localDiscoveryEntries = new HashSet<>();
        final List<GlobalDiscoveryEntry> remoteDiscoveryEntries = new ArrayList<>();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                           localAndCacheDomain,
                                                           interfaceName,
                                                           "participantIdLocal",
                                                           new ProviderQos(),
                                                           System.currentTimeMillis(),
                                                           System.currentTimeMillis() + 100000,
                                                           "publicKeyId");
        DiscoveryEntry cachedDiscoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                                 localAndCacheDomain,
                                                                 interfaceName,
                                                                 "participantIdCached",
                                                                 new ProviderQos(),
                                                                 System.currentTimeMillis(),
                                                                 System.currentTimeMillis() + 100000,
                                                                 "publicKeyId");
        DiscoveryEntry remoteDiscoveryEntry = new DiscoveryEntry(VersionUtil.getVersionFromAnnotation(testProxy.class),
                                                                 remoteDomain,
                                                                 interfaceName,
                                                                 "participantIdRemote",
                                                                 new ProviderQos(),
                                                                 System.currentTimeMillis(),
                                                                 System.currentTimeMillis() + 100000,
                                                                 "publicKeyId");
        localDiscoveryEntries.add(discoveryEntry);
        remoteDiscoveryEntries.add(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(remoteDiscoveryEntry,
                                                                                       globalAddress));

        Set<DiscoveryEntryWithMetaInfo> discoveryEntriesWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfoSet(true,
                                                                                                                              localDiscoveryEntries);
        discoveryEntriesWithMetaInfo.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                             cachedDiscoveryEntry));
        discoveryEntriesWithMetaInfo.add(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                             remoteDiscoveryEntry));

        doReturn(localDiscoveryEntries).when(localDiscoveryEntryStoreMock).lookup(any(String[].class),
                                                                                  eq(interfaceName));
        doReturn(Optional.empty()).when(localDiscoveryEntryStoreMock).lookup(anyString(), anyLong());
        doReturn(Arrays.asList(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(cachedDiscoveryEntry,
                                                                                   globalAddress))).when(globalDiscoveryEntryCacheMock)
                                                                                                   .lookup(eq(testDomains.toArray(new String[0])),
                                                                                                           eq(interfaceName),
                                                                                                           eq(discoveryQos.getCacheMaxAgeMs()));
        doReturn(Optional.of(CapabilityUtils.discoveryEntry2GlobalDiscoveryEntry(cachedDiscoveryEntry,
                                                                                 globalAddress))).when(globalDiscoveryEntryCacheMock)
                                                                                                 .lookup(eq(cachedDiscoveryEntry.getParticipantId()),
                                                                                                         eq(Long.MAX_VALUE));
        doAnswer(createLookupAnswer(remoteDiscoveryEntries)).when(globalCapabilitiesDirectoryClientMock)
                                                            .lookup(ArgumentMatchers.<CallbackWithModeledError<List<GlobalDiscoveryEntry>, DiscoveryError>> any(),
                                                                    any(String[].class),
                                                                    eq(interfaceName),
                                                                    anyLong(),
                                                                    any(String[].class));

        runtime = injector.getInstance(JoynrRuntime.class);
        ProxyBuilder<testProxy> proxyBuilder = runtime.getProxyBuilder(testDomains, testProxy.class);
        final Future<Void> future = new Future<Void>();

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
                                                              any(MessagingQos.class),
                                                              eq(null));

            verifyGlobalLookup(interfaceName, testDomains.toArray(new String[testDomains.size()]));
        } catch (Exception e) {
            Assert.fail("Unexpected exception from ProxyCreatedCallback: " + e);
        }
    }

}
