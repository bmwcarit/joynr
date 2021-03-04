/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.arbitration.ArbitrationCallback;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.Arbitrator;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyTest.TestInterface;
import io.joynr.runtime.ShutdownNotifier;
import io.joynr.util.ObjectMapper;
import io.joynr.util.VersionUtil;
import joynr.tests.testProxy;
import joynr.types.DiscoveryEntry;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Version;

@RunWith(MockitoJUnitRunner.class)
public class GuidedProxyBuilderTest {

    @Mock
    private Arbitrator arbitrator;

    @Mock
    private LocalDiscoveryAggregator localDiscoveryAggregator;

    @Mock
    private ProxyInvocationHandlerFactory proxyInvocationHandlerFactory;

    @Mock
    private StatelessAsyncCallbackDirectory statelessAsyncCallbackDirectory;

    @Mock
    private ProxyInvocationHandler proxyInvocationHandler;

    @Mock
    private ProxyBuilderFactory proxyBuilderFactory;

    @Mock
    private ProxyBuilder proxyBuilder;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Mock
    private ObjectMapper objectMapper;

    private GuidedProxyBuilder subject;
    private Set<String> domains;

    private static final long MAX_MESSAGE_TTL = 1024L;
    private static final long DEFAULT_DISCOVERY_TIMEOUT_MS = 10000L;
    private static final long DEFAULT_RETRY_INTERVAL_MS = 5000L;

    public void setup() throws Exception {
        initializeSubject();
        Field arbitratorField = GuidedProxyBuilder.class.getDeclaredField("arbitrator");
        arbitratorField.setAccessible(true);
        arbitratorField.set(subject, arbitrator);
        when(proxyBuilderFactory.get(Mockito.<Set<String>> any(), any())).thenReturn(proxyBuilder);
        when(proxyInvocationHandlerFactory.create(Mockito.<Set<String>> any(),
                                                  eq(TestInterface.INTERFACE_NAME),
                                                  Mockito.<String> any(),
                                                  Mockito.<DiscoveryQos> any(),
                                                  Mockito.<MessagingQos> any(),
                                                  Mockito.<ShutdownNotifier> any(),
                                                  Mockito.<Optional<StatelessAsyncCallback>> any())).thenReturn(proxyInvocationHandler);
    }

    private void initializeSubject() {
        domains = new HashSet<>();
        domains.add("domain1");
        domains.add("domain2");
        DiscoverySettingsStorage discoverySettingsStorage = new DiscoverySettingsStorage(proxyBuilderFactory,
                                                                                         objectMapper,
                                                                                         localDiscoveryAggregator,
                                                                                         MAX_MESSAGE_TTL,
                                                                                         DEFAULT_DISCOVERY_TIMEOUT_MS,
                                                                                         DEFAULT_RETRY_INTERVAL_MS);
        subject = new GuidedProxyBuilder(discoverySettingsStorage, domains, testProxy.class);
    }

    @Test
    public void testLookupIsProperlyCalled() throws Exception {
        setup();
        CompletableFuture<DiscoveryResult> result = subject.discoverAsync();
        ArgumentCaptor<ArbitrationCallback> callbackCaptor = ArgumentCaptor.forClass(ArbitrationCallback.class);
        verify(arbitrator).setArbitrationListener(callbackCaptor.capture());
        verify(arbitrator).lookup();
        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        String testParticipantId = "test";
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        Set<DiscoveryEntryWithMetaInfo> mockedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(mockedDiscoveryEntry));
        ArbitrationResult mockedArbitrationResult = new ArbitrationResult(mockedSelectedDiscoveryEntries, null);
        callbackCaptor.getValue().onSuccess(mockedArbitrationResult);
        Collection<DiscoveryEntry> resultEntries = result.get().getAllDiscoveryEntries();
        assertEquals(1, resultEntries.size());
        for (DiscoveryEntry entry : resultEntries) {
            assertEquals(testParticipantId, entry.getParticipantId());
        }
    }

    @Test(expected = IllegalStateException.class)
    public void testSetDiscoveryQosThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        subject.setDiscoveryQos(new DiscoveryQos());
    }

    @Test(expected = IllegalStateException.class)
    public void testSetMessagingQosThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        subject.setMessagingQos(new MessagingQos());
    }

    @Test(expected = IllegalStateException.class)
    public void testSetStatelessAsyncCallbackUseCaseThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        subject.setStatelessAsyncCallbackUseCase("Test");
    }

    @Test(expected = IllegalStateException.class)
    public void testSetGbidsThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        String[] gbids = new String[]{};
        subject.setGbids(gbids);
    }

    @Test(expected = ExecutionException.class)
    public void testLookupExceptionIsProperlyThrown() throws Exception {
        setup();
        CompletableFuture<DiscoveryResult> result = subject.discoverAsync();
        ArgumentCaptor<ArbitrationCallback> callbackCaptor = ArgumentCaptor.forClass(ArbitrationCallback.class);
        verify(arbitrator).setArbitrationListener(callbackCaptor.capture());
        verify(arbitrator).lookup();
        callbackCaptor.getValue().onError(new JoynrRuntimeException());
        result.get();
    }

    @Test(expected = ExecutionException.class)
    public void testLookupExceptionIsProperlyThrownNonJoynrRuntimeException() throws Exception {
        setup();
        CompletableFuture<DiscoveryResult> result = subject.discoverAsync();
        ArgumentCaptor<ArbitrationCallback> callbackCaptor = ArgumentCaptor.forClass(ArbitrationCallback.class);
        verify(arbitrator).setArbitrationListener(callbackCaptor.capture());
        verify(arbitrator).lookup();
        callbackCaptor.getValue().onError(new NullPointerException());
        result.get();
    }

    @Test
    public void testLookupIsProperlyCalledSync() throws Exception {
        setup();

        ArbitrationCallback[] callbacks = new ArbitrationCallback[1];
        Mockito.doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                ArbitrationCallback cb = (ArbitrationCallback) invocation.getArguments()[0];
                callbacks[0] = cb;
                return null;
            }
        }).when(arbitrator).setArbitrationListener(Mockito.<ArbitrationCallback> any());

        Mockito.doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                assertTrue(callbacks.length == 1 && callbacks[0] != null);

                DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
                String testParticipantId = "test";
                mockedDiscoveryEntry.setParticipantId(testParticipantId);
                Set<DiscoveryEntryWithMetaInfo> mockedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(mockedDiscoveryEntry));
                ArbitrationResult mockedArbitrationResult = new ArbitrationResult(mockedSelectedDiscoveryEntries, null);

                callbacks[0].onSuccess(mockedArbitrationResult);
                return null;
            }
        }).when(arbitrator).lookup();

        DiscoveryResult result = subject.discover();

        verify(arbitrator).lookup();
        assertNotNull(result);
    }

    @Test
    public void testArbitratorIsProperlyBuilt() throws Exception {
        initializeSubject();
        Field arbitratorField = GuidedProxyBuilder.class.getDeclaredField("arbitrator");
        arbitratorField.setAccessible(true);
        subject.discoverAsync();
        assertNotNull(arbitratorField.get(subject));

        Field discoveryQosField = GuidedProxyBuilder.class.getDeclaredField("discoveryQos");
        discoveryQosField.setAccessible(true);
        DiscoveryQos internalDiscoveryQos = (DiscoveryQos) discoveryQosField.get(subject);
        assert (internalDiscoveryQos.getDiscoveryTimeoutMs() != DiscoveryQos.NO_VALUE);
        assert (internalDiscoveryQos.getRetryIntervalMs() != DiscoveryQos.NO_VALUE);
    }

    @Test
    public void testProxyBuilderIsProperlyCalled() throws Exception {
        setup();
        String testParticipantId = "test";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        subject.setDiscoveryQos(discoveryQos);
        subject.buildProxy(testProxy.class, testParticipantId);
        verify(proxyBuilderFactory).get(Mockito.<Set<String>> any(), eq(testProxy.class));
        verify(proxyBuilder).build(Mockito.<ArbitrationResult> any());

        Field discoveryQosField = GuidedProxyBuilder.class.getDeclaredField("discoveryQos");
        discoveryQosField.setAccessible(true);
        DiscoveryQos internalDiscoveryQos = (DiscoveryQos) discoveryQosField.get(subject);
        assert (internalDiscoveryQos.getDiscoveryTimeoutMs() != DiscoveryQos.NO_VALUE);
        assert (internalDiscoveryQos.getRetryIntervalMs() != DiscoveryQos.NO_VALUE);
    }

    @Test
    public void testProxyBuilderIsProperlyCalledWithBuilderMethods() throws Exception {
        setup();
        String testParticipantId = "test";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        MessagingQos messagingQos = new MessagingQos();
        String statelessAsyncCallbackUseCase = "testUseCase";
        String[] gbids = new String[]{ "gbid1", "gbid2" };
        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        long originalDiscoveryTimeoutMs = 42L;
        ArbitrationStrategy originalArbitrationStrategy = ArbitrationStrategy.HighestPriority;
        long originalCacheMaxAgeMs = 42000L;
        boolean originalProviderMustSupportOnChange = true;
        long originalRetryIntervalMs = 420L;
        DiscoveryScope originalDiscoveryScope = DiscoveryScope.LOCAL_ONLY;

        discoveryQos.setDiscoveryTimeoutMs(originalDiscoveryTimeoutMs);
        discoveryQos.setArbitrationStrategy(originalArbitrationStrategy);
        discoveryQos.setCacheMaxAgeMs(originalCacheMaxAgeMs);
        discoveryQos.setProviderMustSupportOnChange(originalProviderMustSupportOnChange);
        discoveryQos.setRetryIntervalMs(originalRetryIntervalMs);
        discoveryQos.setDiscoveryScope(originalDiscoveryScope);

        subject.setDiscoveryQos(discoveryQos);
        subject.setMessagingQos(messagingQos);
        subject.setStatelessAsyncCallbackUseCase(statelessAsyncCallbackUseCase);
        subject.setGbids(gbids);

        discoveryQos.setDiscoveryTimeoutMs(24L);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.Keyword);
        discoveryQos.setCacheMaxAgeMs(24000L);
        discoveryQos.setProviderMustSupportOnChange(false);
        discoveryQos.setRetryIntervalMs(240L);
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);

        ArgumentCaptor<DiscoveryQos> discoveryQosCaptor = ArgumentCaptor.forClass(DiscoveryQos.class);

        subject.buildProxy(testProxy.class, testParticipantId);
        verify(proxyBuilderFactory).get(Mockito.<Set<String>> any(), eq(testProxy.class));
        verify(proxyBuilder).setDiscoveryQos(discoveryQosCaptor.capture());
        verify(proxyBuilder).setMessagingQos(eq(messagingQos));
        verify(proxyBuilder).setStatelessAsyncCallbackUseCase(eq(statelessAsyncCallbackUseCase));
        verify(proxyBuilder).setGbids(eq(gbids));
        verify(proxyBuilder).build(Mockito.<ArbitrationResult> any());

        assertEquals(originalDiscoveryTimeoutMs, discoveryQosCaptor.getValue().getDiscoveryTimeoutMs());
        assertEquals(originalArbitrationStrategy, discoveryQosCaptor.getValue().getArbitrationStrategy());
        assertEquals(originalCacheMaxAgeMs, discoveryQosCaptor.getValue().getCacheMaxAgeMs());
        assertEquals(originalProviderMustSupportOnChange,
                     discoveryQosCaptor.getValue().getProviderMustSupportOnChange());
        assertEquals(originalRetryIntervalMs, discoveryQosCaptor.getValue().getRetryIntervalMs());
        assertEquals(originalDiscoveryScope, discoveryQosCaptor.getValue().getDiscoveryScope());
    }

    private void mockSuccessfulDiscovery(DiscoveryEntryWithMetaInfo mockedDiscoveryEntry) throws NoSuchFieldException,
                                                                                          IllegalAccessException {
        Set<DiscoveryEntryWithMetaInfo> mockedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(mockedDiscoveryEntry));
        ArbitrationResult mockedArbitrationResult = new ArbitrationResult(mockedSelectedDiscoveryEntries, null);

        Field discoveryCompletedFiled = GuidedProxyBuilder.class.getDeclaredField("discoveryCompletedOnce");
        discoveryCompletedFiled.setAccessible(true);
        discoveryCompletedFiled.set(subject, true);

        Field arbitrationResultField = GuidedProxyBuilder.class.getDeclaredField("savedArbitrationResult");
        arbitrationResultField.setAccessible(true);
        arbitrationResultField.set(subject, mockedArbitrationResult);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuildProxyThrowsOnIncompatibleVersion() throws Exception {
        setup();
        String testParticipantId = "test";
        DiscoveryQos discoveryQos = new DiscoveryQos();

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(new Version(500, 600));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        subject.setDiscoveryQos(discoveryQos);
        subject.buildProxy(testProxy.class, testParticipantId);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuildProxyThrowsOnUnavailableParticipantId() throws Exception {
        setup();
        String testParticipantId = "test";
        DiscoveryQos discoveryQos = new DiscoveryQos();

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        subject.setDiscoveryQos(discoveryQos);
        subject.buildProxy(testProxy.class, "unavailableParticipantId");
    }

}
