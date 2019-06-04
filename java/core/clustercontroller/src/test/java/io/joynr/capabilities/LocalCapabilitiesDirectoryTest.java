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
package io.joynr.capabilities;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.argThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.after;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.timeout;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.hamcrest.Description;
import org.hamcrest.Matcher;
import org.hamcrest.Matchers;
import org.hamcrest.TypeSafeMatcher;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.internal.matchers.VarargMatcher;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.fasterxml.jackson.databind.ObjectMapper;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.dispatching.Dispatcher;
import io.joynr.exceptions.JoynrException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilderFactory;
import io.joynr.runtime.GlobalAddressProvider;
import io.joynr.runtime.JoynrRuntime;
import io.joynr.runtime.ShutdownNotifier;
import joynr.infrastructure.GlobalCapabilitiesDirectory;
import joynr.infrastructure.GlobalDomainAccessController;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.CustomParameter;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class LocalCapabilitiesDirectoryTest {
    private static final String TEST_URL = "http://testUrl";
    private static final long ONE_DAY_IN_MS = 1 * 24 * 60 * 60 * 1000;
    private static final long defaultDiscoveryRetryIntervalMs = 2000L;
    private Long expiryDateMs = System.currentTimeMillis() + ONE_DAY_IN_MS;
    private String publicKeyId = "publicKeyId";

    @Mock
    JoynrRuntime runtime;
    @Mock
    private GlobalCapabilitiesDirectoryClient globalCapabilitiesClient;
    @Mock
    private ExpiredDiscoveryEntryCacheCleaner expiredDiscoveryEntryCacheCleaner;
    @Mock
    private MessageRouter messageRouter;
    @Mock
    private Dispatcher dispatcher;
    @Mock
    private ProxyBuilderFactory proxyBuilderFactoryMock;
    @Mock
    private DiscoveryEntryStore<DiscoveryEntry> localDiscoveryEntryStoreMock;
    @Mock
    private DiscoveryEntryStore<GlobalDiscoveryEntry> globalDiscoveryEntryCacheMock;
    @Mock
    private GlobalAddressProvider globalAddressProvider;
    @Mock
    private CapabilitiesProvisioning capabilitiesProvisioning;
    @Mock
    private ScheduledExecutorService capabilitiesFreshnessUpdateExecutor;
    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Captor
    private ArgumentCaptor<Collection<DiscoveryEntryWithMetaInfo>> capabilitiesCaptor;

    @Captor
    private ArgumentCaptor<Runnable> runnableCaptor;

    private LocalCapabilitiesDirectory localCapabilitiesDirectory;
    private ChannelAddress channelAddress;
    private String channelAddressSerialized;
    private DiscoveryEntry discoveryEntry;
    private GlobalDiscoveryEntry globalDiscoveryEntry;

    public interface TestInterface {
        public static final String INTERFACE_NAME = "interfaceName";
    }

    private static class DiscoveryEntryStoreVarargMatcher
            extends ArgumentMatcher<DiscoveryEntryStore<? extends DiscoveryEntry>[]> implements VarargMatcher {
        private static final long serialVersionUID = 1L;
        private final DiscoveryEntryStore<? extends DiscoveryEntry>[] matchAgainst;

        private DiscoveryEntryStoreVarargMatcher(DiscoveryEntryStore<?>... matchAgainst) {
            this.matchAgainst = matchAgainst;
        }

        @Override
        public boolean matches(Object argument) {
            assertNotNull(argument);
            assertArrayEquals(matchAgainst, (DiscoveryEntryStore[]) argument);
            return true;
        }
    }

    @SuppressWarnings("unchecked")
    @Before
    public void setUp() throws Exception {

        channelAddress = new ChannelAddress(TEST_URL, "testChannelId");
        ObjectMapper objectMapper = new ObjectMapper();
        channelAddressSerialized = objectMapper.writeValueAsString(channelAddress);

        Field objectMapperField = CapabilityUtils.class.getDeclaredField("objectMapper");
        objectMapperField.setAccessible(true);
        objectMapperField.set(CapabilityUtils.class, objectMapper);

        Answer<Void> answer = new Answer<Void>() {

            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                return null;
            }

        };
        doAnswer(answer).when(globalCapabilitiesClient).add(any(Callback.class), any(GlobalDiscoveryEntry.class));

        String discoveryDirectoriesDomain = "io.joynr";
        String capabilitiesDirectoryParticipantId = "capDir_participantId";
        String capabiltitiesDirectoryChannelId = "dirchannelId";
        String domainAccessControllerParticipantId = "domainAccessControllerParticipantId";
        String domainAccessControllerChannelId = "domainAccessControllerChannelId";
        GlobalDiscoveryEntry globalCapabilitiesDirectoryDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0,
                                                                                                                             1),
                                                                                                                 discoveryDirectoriesDomain,
                                                                                                                 GlobalCapabilitiesDirectory.INTERFACE_NAME,
                                                                                                                 capabilitiesDirectoryParticipantId,
                                                                                                                 new ProviderQos(),
                                                                                                                 System.currentTimeMillis(),
                                                                                                                 expiryDateMs,
                                                                                                                 domainAccessControllerChannelId,
                                                                                                                 new ChannelAddress(TEST_URL,
                                                                                                                                    capabiltitiesDirectoryChannelId));

        GlobalDiscoveryEntry domainAccessControllerDiscoveryEntry = CapabilityUtils.newGlobalDiscoveryEntry(new Version(0,
                                                                                                                        1),
                                                                                                            discoveryDirectoriesDomain,
                                                                                                            GlobalDomainAccessController.INTERFACE_NAME,
                                                                                                            domainAccessControllerParticipantId,
                                                                                                            new ProviderQos(),
                                                                                                            System.currentTimeMillis(),
                                                                                                            expiryDateMs,
                                                                                                            domainAccessControllerChannelId,
                                                                                                            new ChannelAddress(TEST_URL,
                                                                                                                               domainAccessControllerChannelId));

        when(capabilitiesProvisioning.getDiscoveryEntries()).thenReturn(new HashSet<GlobalDiscoveryEntry>(Arrays.asList(globalCapabilitiesDirectoryDiscoveryEntry,
                                                                                                                        domainAccessControllerDiscoveryEntry)));
        // use default freshnessUpdateIntervalMs: 3600000ms (1h)
        localCapabilitiesDirectory = new LocalCapabilitiesDirectoryImpl(capabilitiesProvisioning,
                                                                        globalAddressProvider,
                                                                        localDiscoveryEntryStoreMock,
                                                                        globalDiscoveryEntryCacheMock,
                                                                        messageRouter,
                                                                        globalCapabilitiesClient,
                                                                        expiredDiscoveryEntryCacheCleaner,
                                                                        3600000,
                                                                        capabilitiesFreshnessUpdateExecutor,
                                                                        defaultDiscoveryRetryIntervalMs,
                                                                        shutdownNotifier);
        verify(expiredDiscoveryEntryCacheCleaner).scheduleCleanUpForCaches(Mockito.<ExpiredDiscoveryEntryCacheCleaner.CleanupAction> any(),
                                                                           argThat(new DiscoveryEntryStoreVarargMatcher(globalDiscoveryEntryCacheMock,
                                                                                                                        localDiscoveryEntryStoreMock)));
        verify(capabilitiesFreshnessUpdateExecutor).scheduleAtFixedRate(runnableCaptor.capture(),
                                                                        anyLong(),
                                                                        anyLong(),
                                                                        eq(TimeUnit.MILLISECONDS));

        ProviderQos providerQos = new ProviderQos();
        CustomParameter[] parameterList = { new CustomParameter("key1", "value1"),
                new CustomParameter("key2", "value2") };
        providerQos.setCustomParameters(parameterList);

        String participantId = "testParticipantId";
        String domain = "domain";
        discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                            domain,
                                            TestInterface.INTERFACE_NAME,
                                            participantId,
                                            providerQos,
                                            System.currentTimeMillis(),
                                            expiryDateMs,
                                            publicKeyId);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        channelAddressSerialized);
    }

    @Test(timeout = 1000)
    public void addCapability() throws InterruptedException {
        when(globalAddressProvider.get()).thenReturn(channelAddress);

        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);

        ArgumentCaptor<GlobalDiscoveryEntry> argumentCaptor = ArgumentCaptor.forClass(GlobalDiscoveryEntry.class);
        verify(globalCapabilitiesClient, timeout(200)).add(any(Callback.class), argumentCaptor.capture());
        GlobalDiscoveryEntry capturedGlobalDiscoveryEntry = argumentCaptor.getValue();
        assertNotNull(capturedGlobalDiscoveryEntry);
        assertEquals(discoveryEntry.getDomain(), capturedGlobalDiscoveryEntry.getDomain());
        assertEquals(discoveryEntry.getInterfaceName(), capturedGlobalDiscoveryEntry.getInterfaceName());
    }

    @Test(timeout = 2000)
    public void addLocalOnlyCapability() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        "test",
                                                        TestInterface.INTERFACE_NAME,
                                                        "participantId",
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        channelAddressSerialized);

        localCapabilitiesDirectory.add(discoveryEntry);
        Thread.sleep(1000);
        verify(globalCapabilitiesClient, never()).add(any(Callback.class), any(GlobalDiscoveryEntry.class));
    }

    @Test(timeout = 1000)
    public void addGlobalCapSucceeds_NextAddShallNotAddGlobalAgain() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName()
                + ".addGlobalCapSucceeds_NextAddShallNotAddGlobalAgain";
        String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 TestInterface.INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis(),
                                                                 expiryDateMs,
                                                                 publicKeyId);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        channelAddressSerialized);

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        promise.then(new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                Mockito.doAnswer(createAddAnswerWithSuccess())
                       .when(globalCapabilitiesClient)
                       .add(any(Callback.class), eq(globalDiscoveryEntry));

                verify(globalDiscoveryEntryCacheMock).add(eq(globalDiscoveryEntry));
                verify(globalCapabilitiesClient).add(any(Callback.class), eq(globalDiscoveryEntry));
                reset(globalCapabilitiesClient);
                localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
                verify(globalCapabilitiesClient, after(200).never()).add(any(Callback.class), eq(globalDiscoveryEntry));
            }

            @Override
            public void onRejection(JoynrException error) {
                Assert.fail("adding capability failed: " + error);
            }
        });

    }

    @Test(timeout = 1000)
    public void addGlobalCapFails_NextAddShallAddGlobalAgain() throws InterruptedException {

        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.GLOBAL);

        String participantId = LocalCapabilitiesDirectoryTest.class.getName() + ".addLocalAndThanGlobalShallWork";
        String domain = "testDomain";
        final DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                                 domain,
                                                                 TestInterface.INTERFACE_NAME,
                                                                 participantId,
                                                                 providerQos,
                                                                 System.currentTimeMillis(),
                                                                 expiryDateMs,
                                                                 publicKeyId);
        globalDiscoveryEntry = new GlobalDiscoveryEntry(new Version(47, 11),
                                                        domain,
                                                        TestInterface.INTERFACE_NAME,
                                                        participantId,
                                                        providerQos,
                                                        System.currentTimeMillis(),
                                                        expiryDateMs,
                                                        publicKeyId,
                                                        channelAddressSerialized);

        Mockito.doAnswer(createAddAnswerWithError()).when(globalCapabilitiesClient).add(any(Callback.class),
                                                                                        eq(globalDiscoveryEntry));

        final boolean awaitGlobalRegistration = true;
        Promise<DeferredVoid> promise = localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        promise.then(new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                verify(globalDiscoveryEntryCacheMock, never()).add(eq(globalDiscoveryEntry));
                verify(globalCapabilitiesClient).add(any(Callback.class), eq(globalDiscoveryEntry));
                reset(globalCapabilitiesClient);
                localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
                verify(globalCapabilitiesClient, timeout(200)).add(any(Callback.class), eq(globalDiscoveryEntry));

            }

            @Override
            public void onRejection(JoynrException error) {

            }
        });

    }

    private Answer<Future<List<GlobalDiscoveryEntry>>> createAnswer(final List<GlobalDiscoveryEntry> caps) {
        return new Answer<Future<List<GlobalDiscoveryEntry>>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<List<GlobalDiscoveryEntry>> answer(InvocationOnMock invocation) throws Throwable {
                Future<List<GlobalDiscoveryEntry>> result = new Future<List<GlobalDiscoveryEntry>>();
                Object[] args = invocation.getArguments();
                ((Callback<List<GlobalDiscoveryEntry>>) args[0]).onSuccess(caps);
                result.onSuccess(caps);
                return result;
            }
        };
    }

    private Answer<Future<Void>> createAddAnswerWithSuccess() {
        return new Answer<Future<Void>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onSuccess(null);
                result.onSuccess(null);
                return result;
            }
        };
    }

    private Answer<Future<Void>> createAddAnswerWithError() {
        return new Answer<Future<Void>>() {

            @SuppressWarnings("unchecked")
            @Override
            public Future<Void> answer(InvocationOnMock invocation) throws Throwable {
                Future<Void> result = new Future<Void>();
                Object[] args = invocation.getArguments();
                ((Callback<Void>) args[0]).onFailure(new JoynrRuntimeException("Simulating a JoynrRuntimeException on callback"));
                result.onSuccess(null);
                return result;
            }
        };
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void lookupWithScopeGlobalOnly() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     1000,
                                                     DiscoveryScope.GLOBAL_ONLY);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        when(globalDiscoveryEntryCacheMock.lookup(eq(new String[]{
                domain1 }), eq(interfaceName1), eq(discoveryQos.getCacheMaxAgeMs())))
                                                                                     .thenReturn(new ArrayList<GlobalDiscoveryEntry>());
        doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(any(Callback.class),
                                                                           eq(new String[]{ domain1 }),
                                                                           eq(interfaceName1),
                                                                           eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(1)).lookup(any(Callback.class),
                                                          any(String[].class),
                                                          any(String.class),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(1)));
        reset(capabilitiesCallback);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        final boolean awaitGlobalRegistration = true;
        localCapabilitiesDirectory.add(discoveryEntry, awaitGlobalRegistration);
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(2)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(1)));
        reset(capabilitiesCallback);

        // even deleting local cap entries shall have no effect, the global cap dir shall be invoked
        localCapabilitiesDirectory.remove(discoveryEntry);
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(3)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(1)));
        reset(capabilitiesCallback);

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                channelAddressSerialized);
        caps.add(capInfo);
        doAnswer(createAnswer(caps)).when(globalCapabilitiesClient).lookup(any(Callback.class),
                                                                           eq(new String[]{ domain1 }),
                                                                           eq(interfaceName1),
                                                                           eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(4)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(1)));
        reset(capabilitiesCallback);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        reset(globalDiscoveryEntryCacheMock);
        when(globalDiscoveryEntryCacheMock.lookup(eq(new String[]{
                domain1 }), eq(interfaceName1), eq(discoveryQos.getCacheMaxAgeMs())))
                                                                                     .thenReturn(Arrays.asList(capInfo));

        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(4)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(1)));
        reset(capabilitiesCallback);

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAgeMs(0);
        Thread.sleep(1);

        // now, another lookup call shall call the globalCapabilitiesClient, as the global cap dir is expired
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(5)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(1)));
        reset(capabilitiesCallback);
        reset(globalCapabilitiesClient);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void lookupWithScopeLocalThenGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     1000,
                                                     DiscoveryScope.LOCAL_THEN_GLOBAL);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(any(Callback.class),
                       eq(new String[]{ domain1 }),
                       eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(1)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(1)));

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        reset(localDiscoveryEntryStoreMock);
        when(localDiscoveryEntryStoreMock.lookup(eq(new String[]{ domain1 }),
                                                 eq(interfaceName1))).thenReturn(Arrays.asList(discoveryEntry));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(1)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(1)));

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                channelAddressSerialized);
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(any(Callback.class),
                       eq(new String[]{ domain1 }),
                       eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(1)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        reset(localDiscoveryEntryStoreMock);
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(2)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        when(globalDiscoveryEntryCacheMock.lookup(eq(new String[]{
                domain1 }), eq(interfaceName1), eq(discoveryQos.getCacheMaxAgeMs())))
                                                                                     .thenReturn(Arrays.asList(capInfo));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(2)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAgeMs(0);
        Thread.sleep(1);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(3)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        reset(globalCapabilitiesClient);
        reset(capabilitiesCallback);
    }

    @Test(timeout = 1000)
    public void lookupByParticipantIdWithScopeLocalSync() throws InterruptedException {
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        String participantId1 = "participantId1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     10000,
                                                     DiscoveryScope.LOCAL_ONLY);

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           participantId1,
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        DiscoveryEntryWithMetaInfo expectedDiscoveryEntry = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                discoveryEntry);
        when(localDiscoveryEntryStoreMock.lookup(eq(participantId1),
                                                 eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(discoveryEntry);
        DiscoveryEntry retrievedCapabilityEntry = localCapabilitiesDirectory.lookup(participantId1, discoveryQos);
        assertEquals(expectedDiscoveryEntry, retrievedCapabilityEntry);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void lookupWithScopeLocalAndGlobal() throws InterruptedException {
        List<GlobalDiscoveryEntry> caps = new ArrayList<GlobalDiscoveryEntry>();
        String domain1 = "domain1";
        String interfaceName1 = "interfaceName1";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     500,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL);
        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);

        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(any(Callback.class),
                       eq(new String[]{ domain1 }),
                       eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(1)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(0)).processCapabilitiesReceived(argThat(hasNEntries(1)));

        // add local entry
        ProviderQos providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain1,
                                                           interfaceName1,
                                                           "localParticipant",
                                                           providerQos,
                                                           System.currentTimeMillis(),
                                                           expiryDateMs,
                                                           publicKeyId);
        when(localDiscoveryEntryStoreMock.lookup(eq(new String[]{ domain1 }),
                                                 eq(interfaceName1))).thenReturn(Arrays.asList(discoveryEntry));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(2)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(0)));
        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(1)));

        // add global entry
        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain1,
                                                                interfaceName1,
                                                                "globalParticipant",
                                                                new ProviderQos(),
                                                                System.currentTimeMillis(),
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                channelAddressSerialized);
        caps.add(capInfo);
        Mockito.doAnswer(createAnswer(caps))
               .when(globalCapabilitiesClient)
               .lookup(any(Callback.class),
                       eq(new String[]{ domain1 }),
                       eq(interfaceName1),
                       eq(discoveryQos.getDiscoveryTimeoutMs()));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(3)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        when(globalDiscoveryEntryCacheMock.lookup(eq(new String[]{
                domain1 }), eq(interfaceName1), eq(discoveryQos.getCacheMaxAgeMs())))
                                                                                     .thenReturn(Arrays.asList(capInfo));
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(3)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));

        // and now, invalidate the existing cached global values, resulting in another call to glocalcapclient
        discoveryQos.setCacheMaxAgeMs(0);
        Thread.sleep(1);

        // now, another lookup call shall take the cached for the global cap call, and no longer call the global cap dir
        // (as long as the cache is not expired)
        localCapabilitiesDirectory.lookup(new String[]{ domain1 }, interfaceName1, discoveryQos, capabilitiesCallback);
        verify(globalCapabilitiesClient, times(4)).lookup(any(Callback.class),
                                                          eq(new String[]{ domain1 }),
                                                          eq(interfaceName1),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
        reset(globalCapabilitiesClient);
        reset(capabilitiesCallback);
    }

    @Test(timeout = 1000)
    public void lookupLocalAndGlobalFiltersDuplicates() throws InterruptedException {
        String domain = "domain";
        String interfaceName = "interfaceName";
        String participant = "participant";
        DiscoveryQos discoveryQos = new DiscoveryQos(30000,
                                                     ArbitrationStrategy.HighestPriority,
                                                     500,
                                                     DiscoveryScope.LOCAL_AND_GLOBAL);

        // add same discovery entry to localCapabilitiesDirectory and cached GlobalCapabilitiesDirectory
        ProviderQos providerQos = new ProviderQos();
        long currentTime = System.currentTimeMillis();
        DiscoveryEntry discoveryEntry = new DiscoveryEntry(new Version(47, 11),
                                                           domain,
                                                           interfaceName,
                                                           participant,
                                                           providerQos,
                                                           currentTime,
                                                           expiryDateMs,
                                                           publicKeyId);

        when(localDiscoveryEntryStoreMock.lookup(eq(new String[]{ domain }),
                                                 eq(interfaceName))).thenReturn(Arrays.asList(discoveryEntry));

        GlobalDiscoveryEntry capInfo = new GlobalDiscoveryEntry(new Version(47, 11),
                                                                domain,
                                                                interfaceName,
                                                                participant,
                                                                providerQos,
                                                                currentTime,
                                                                expiryDateMs,
                                                                publicKeyId,
                                                                channelAddressSerialized);

        when(globalDiscoveryEntryCacheMock.lookup(eq(new String[]{
                domain }), eq(interfaceName), eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(Arrays.asList(capInfo));

        CapabilitiesCallback capabilitiesCallback = Mockito.mock(CapabilitiesCallback.class);
        localCapabilitiesDirectory.lookup(new String[]{ domain }, interfaceName, discoveryQos, capabilitiesCallback);

        verify(capabilitiesCallback, times(1)).processCapabilitiesReceived(argThat(hasNEntries(1)));

        verify(capabilitiesCallback).processCapabilitiesReceived(capabilitiesCaptor.capture());
        Collection<DiscoveryEntryWithMetaInfo> discoveredEntries = capabilitiesCaptor.getValue();
        assertTrue(discoveredEntries.contains(CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                  discoveryEntry)));

        reset(globalCapabilitiesClient);
        reset(capabilitiesCallback);
    }

    @Test
    public void testLookupMultipleDomainsLocalOnly() {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        Collection<DiscoveryEntry> entries = Arrays.asList(new DiscoveryEntry(new Version(0, 0),
                                                                              "domain1",
                                                                              interfaceName,
                                                                              "participantId1",
                                                                              new ProviderQos(),
                                                                              System.currentTimeMillis(),
                                                                              expiryDateMs,
                                                                              interfaceName),
                                                           new DiscoveryEntry(new Version(0, 0),
                                                                              "domain2",
                                                                              interfaceName,
                                                                              "participantId2",
                                                                              new ProviderQos(),
                                                                              System.currentTimeMillis(),
                                                                              expiryDateMs,
                                                                              interfaceName));
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(entries);

        localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos, capabilitiesCallback);

        verify(capabilitiesCallback).processCapabilitiesReceived(capabilitiesCaptor.capture());
        Collection<DiscoveryEntry> discoveredEntries = CapabilityUtils.convertToDiscoveryEntryList(capabilitiesCaptor.getValue());
        assertNotNull(discoveredEntries);
        assertEquals(2, discoveredEntries.size());
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testLookupMultipleDomainsGlobalOnly() {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(new ArrayList<GlobalDiscoveryEntry>());

        localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos, capabilitiesCallback);

        verify(globalCapabilitiesClient).lookup(any(Callback.class),
                                                argThat(Matchers.arrayContainingInAnyOrder(domains)),
                                                eq(interfaceName),
                                                eq(discoveryQos.getDiscoveryTimeoutMs()));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testLookupMultipleDomainsGlobalOnlyAllCached() {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        List<GlobalDiscoveryEntry> entries = new ArrayList<>();
        for (String domain : domains) {
            GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
            entry.setDomain(domain);
            entries.add(entry);
        }

        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(entries);

        localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos, capabilitiesCallback);

        verify(globalCapabilitiesClient, times(0)).lookup(any(Callback.class),
                                                          argThat(Matchers.arrayContainingInAnyOrder(domains)),
                                                          eq(interfaceName),
                                                          eq(discoveryQos.getDiscoveryTimeoutMs()));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testLookupMultipleDomainsGlobalOnlyOneCached() {
        String[] domains = new String[]{ "domain1", "domain2" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setCacheMaxAgeMs(ONE_DAY_IN_MS);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        GlobalDiscoveryEntry entry = new GlobalDiscoveryEntry();
        entry.setDomain("domain1");
        Collection<GlobalDiscoveryEntry> entries = Arrays.asList(entry);
        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(entries);

        localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos, capabilitiesCallback);

        verify(globalCapabilitiesClient).lookup(any(Callback.class),
                                                argThat(Matchers.arrayContainingInAnyOrder(new String[]{ "domain2" })),
                                                eq(interfaceName),
                                                eq(discoveryQos.getDiscoveryTimeoutMs()));
    }

    @SuppressWarnings("unchecked")
    @Test
    public void testLookupMultipleDomainsLocalThenGlobal() {
        String[] domains = new String[]{ "domain1", "domain2", "domain3" };
        String interfaceName = "interface1";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        discoveryQos.setCacheMaxAgeMs(ONE_DAY_IN_MS);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setDomain("domain1");
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(Arrays.asList(localEntry));

        GlobalDiscoveryEntry globalEntry = new GlobalDiscoveryEntry();
        globalEntry.setDomain("domain2");
        Collection<GlobalDiscoveryEntry> entries = Arrays.asList(globalEntry);
        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(entries);

        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                "domain3",
                                                                                interfaceName,
                                                                                "participantIdRemote",
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                channelAddressSerialized);

        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                Callback<List<GlobalDiscoveryEntry>> callback = (Callback<List<GlobalDiscoveryEntry>>) invocation.getArguments()[0];
                callback.onSuccess(Arrays.asList(remoteGlobalEntry));
                return null;
            }
        }).when(globalCapabilitiesClient).lookup(any(Callback.class), Mockito.<String[]> any(), anyString(), anyLong());

        localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos, capabilitiesCallback);

        verify(globalCapabilitiesClient).lookup(any(Callback.class),
                                                argThat(Matchers.arrayContainingInAnyOrder(new String[]{ "domain3" })),
                                                eq(interfaceName),
                                                eq(discoveryQos.getDiscoveryTimeoutMs()));
        verify(capabilitiesCallback).processCapabilitiesReceived(capabilitiesCaptor.capture());
        Collection<DiscoveryEntry> captured = CapabilityUtils.convertToDiscoveryEntrySet(capabilitiesCaptor.getValue());
        assertNotNull(captured);
        assertEquals(3, captured.size());
        assertTrue(captured.contains(localEntry));
        assertTrue(captured.contains(new DiscoveryEntry(globalEntry)));
        assertTrue(captured.contains(new DiscoveryEntry(remoteGlobalEntry)));
    }

    @Test
    public void testLookupByParticipantId_localEntry_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue() {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // local DiscoveryEntry
        String localDomain = "localDomain";
        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setDomain(localDomain);
        localEntry.setInterfaceName(interfaceName);
        localEntry.setParticipantId(participantId);
        when(localDiscoveryEntryStoreMock.lookup(eq(participantId),
                                                 eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(localEntry);

        DiscoveryEntryWithMetaInfo capturedLocalEntry = localCapabilitiesDirectory.lookup(participantId, discoveryQos);
        DiscoveryEntryWithMetaInfo localEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                localEntry);
        assertEquals(localEntryWithMetaInfo, capturedLocalEntry);
    }

    @Test
    public void testLookupByParticipantId_cachedEntry_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue() {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // cached global DiscoveryEntry
        String globalDomain = "globalDomain";
        GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setDomain(globalDomain);
        cachedGlobalEntry.setInterfaceName(interfaceName);
        cachedGlobalEntry.setParticipantId(participantId);
        when(globalDiscoveryEntryCacheMock.lookup(eq(participantId),
                                                  eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(cachedGlobalEntry);

        DiscoveryEntryWithMetaInfo capturedCachedGlobalEntry = localCapabilitiesDirectory.lookup(participantId,
                                                                                                 discoveryQos);
        DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       cachedGlobalEntry);
        assertEquals(cachedGlobalEntryWithMetaInfo, capturedCachedGlobalEntry);
    }

    @Test
    public void testLookupByParticipantId_globalEntry_DiscoveryEntryWithMetaInfoContainsExpectedIsLocalValue() {
        String participantId = "participantId";
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);

        // remote global DiscoveryEntry
        String remoteGlobalDomain = "remoteglobaldomain";
        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                remoteGlobalDomain,
                                                                                interfaceName,
                                                                                participantId,
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                channelAddressSerialized);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<GlobalDiscoveryEntry> callback = (Callback<GlobalDiscoveryEntry>) invocation.getArguments()[0];
                callback.onSuccess(remoteGlobalEntry);
                return null;
            }
        }).when(globalCapabilitiesClient)
          .lookup(org.mockito.Matchers.<Callback<GlobalDiscoveryEntry>> any(), eq(participantId), anyLong());

        DiscoveryEntryWithMetaInfo capturedRemoteGlobalEntry = localCapabilitiesDirectory.lookup(participantId,
                                                                                                 discoveryQos);
        DiscoveryEntryWithMetaInfo remoteGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       remoteGlobalEntry);
        assertEquals(remoteGlobalEntryWithMetaInfo, capturedRemoteGlobalEntry);
    }

    @Test
    public void testLookup_DiscoveryEntriesWithMetaInfoContainExpectedIsLocalValue() {
        String globalDomain = "globaldomain";
        String remoteGlobalDomain = "remoteglobaldomain";
        String[] domains = new String[]{ "localdomain", globalDomain, remoteGlobalDomain };
        String interfaceName = "interfaceName";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_THEN_GLOBAL);
        CapabilitiesCallback capabilitiesCallback = mock(CapabilitiesCallback.class);

        // local DiscoveryEntry
        DiscoveryEntry localEntry = new DiscoveryEntry();
        localEntry.setDomain(domains[0]);
        when(localDiscoveryEntryStoreMock.lookup(eq(domains), eq(interfaceName))).thenReturn(Arrays.asList(localEntry));

        // cached global DiscoveryEntry
        GlobalDiscoveryEntry cachedGlobalEntry = new GlobalDiscoveryEntry();
        cachedGlobalEntry.setDomain(globalDomain);
        Collection<GlobalDiscoveryEntry> cachedEntries = Arrays.asList(cachedGlobalEntry);
        when(globalDiscoveryEntryCacheMock.lookup(eq(domains),
                                                  eq(interfaceName),
                                                  eq(discoveryQos.getCacheMaxAgeMs()))).thenReturn(cachedEntries);

        // remote global DiscoveryEntry
        final GlobalDiscoveryEntry remoteGlobalEntry = new GlobalDiscoveryEntry(new Version(0, 0),
                                                                                remoteGlobalDomain,
                                                                                interfaceName,
                                                                                "participantIdRemote",
                                                                                new ProviderQos(),
                                                                                System.currentTimeMillis(),
                                                                                System.currentTimeMillis() + 10000L,
                                                                                "publicKeyId",
                                                                                channelAddressSerialized);
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                @SuppressWarnings("unchecked")
                Callback<List<GlobalDiscoveryEntry>> callback = (Callback<List<GlobalDiscoveryEntry>>) invocation.getArguments()[0];
                callback.onSuccess(Arrays.asList(remoteGlobalEntry));
                return null;
            }
        }).when(globalCapabilitiesClient).lookup(org.mockito.Matchers.<Callback<List<GlobalDiscoveryEntry>>> any(),
                                                 eq(new String[]{ remoteGlobalDomain }),
                                                 eq(interfaceName),
                                                 anyLong());

        localCapabilitiesDirectory.lookup(domains, interfaceName, discoveryQos, capabilitiesCallback);

        verify(capabilitiesCallback).processCapabilitiesReceived(capabilitiesCaptor.capture());
        Collection<DiscoveryEntryWithMetaInfo> captured = capabilitiesCaptor.getValue();
        assertNotNull(captured);
        assertEquals(3, captured.size());

        DiscoveryEntryWithMetaInfo localEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(true,
                                                                                                                localEntry);
        assertTrue(captured.contains(localEntryWithMetaInfo));

        DiscoveryEntryWithMetaInfo cachedGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       cachedGlobalEntry);
        assertTrue(captured.contains(cachedGlobalEntryWithMetaInfo));
        DiscoveryEntryWithMetaInfo remoteGlobalEntryWithMetaInfo = CapabilityUtils.convertToDiscoveryEntryWithMetaInfo(false,
                                                                                                                       remoteGlobalEntry);
        assertTrue(captured.contains(remoteGlobalEntryWithMetaInfo));

    }

    private class MyCollectionMatcher extends TypeSafeMatcher<Collection<DiscoveryEntryWithMetaInfo>> {

        private int n;

        public MyCollectionMatcher(int n) {
            this.n = n;
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("list has " + n + " entries");
        }

        @Override
        protected boolean matchesSafely(Collection<DiscoveryEntryWithMetaInfo> item) {
            return item.size() == n;
        }

    }

    Matcher<Collection<DiscoveryEntryWithMetaInfo>> hasNEntries(int n) {
        return new MyCollectionMatcher(n);
    }

    @SuppressWarnings("unchecked")
    @Test(timeout = 1000)
    public void removeCapabilities() throws InterruptedException {
        localCapabilitiesDirectory.remove(discoveryEntry);

        verify(globalCapabilitiesClient,
               timeout(1000)).remove(any(Callback.class), eq(Arrays.asList(globalDiscoveryEntry.getParticipantId())));
    }

    @Test
    public void callTouchPeriodically() throws InterruptedException {
        Runnable runnable = runnableCaptor.getValue();
        runnable.run();
        verify(globalCapabilitiesClient).touch();
    }
}
