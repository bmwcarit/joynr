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
import static org.junit.Assert.assertThrows;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.never;
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
import java.util.function.Consumer;
import java.util.function.Function;

import org.junit.Test;
import org.junit.function.ThrowingRunnable;
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
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.routing.MessageRouter;
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

    @SuppressWarnings("rawtypes")
    @Mock
    private ProxyBuilder proxyBuilder;

    @Mock
    private MessageRouter messageRouter;

    @Mock
    private ShutdownNotifier shutdownNotifier;

    @Mock
    private ObjectMapper objectMapper;

    private GuidedProxyBuilder subject;
    private Set<String> domains;

    private static final long MAX_MESSAGE_TTL = 1024L;
    private static final long DEFAULT_DISCOVERY_TIMEOUT_MS = 10000L;
    private static final long DEFAULT_RETRY_INTERVAL_MS = 5000L;

    @SuppressWarnings("unchecked")
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
        subject = new GuidedProxyBuilder(discoverySettingsStorage, domains, testProxy.class, messageRouter);
    }

    @Test
    public void testLookupIsProperlyCalled() throws Exception {
        setup();
        CompletableFuture<DiscoveryResult> result = subject.discoverAsync();
        ArgumentCaptor<ArbitrationCallback> callbackCaptor = ArgumentCaptor.forClass(ArbitrationCallback.class);
        verify(arbitrator).setArbitrationListener(callbackCaptor.capture());
        verify(arbitrator).scheduleArbitration(false);
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

    @Test(expected = JoynrIllegalStateException.class)
    public void testSetDiscoveryQosThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        subject.setDiscoveryQos(new DiscoveryQos());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testSetMessagingQosThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        subject.setMessagingQos(new MessagingQos());
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testSetStatelessAsyncCallbackUseCaseThrowsDuringLookup() throws Exception {
        setup();
        subject.discoverAsync();
        subject.setStatelessAsyncCallbackUseCase("Test");
    }

    @Test(expected = JoynrIllegalStateException.class)
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
        verify(arbitrator).scheduleArbitration(false);
        callbackCaptor.getValue().onError(new JoynrRuntimeException());
        result.get();
    }

    @Test(expected = ExecutionException.class)
    public void testLookupExceptionIsProperlyThrownNonJoynrRuntimeException() throws Exception {
        setup();
        CompletableFuture<DiscoveryResult> result = subject.discoverAsync();
        ArgumentCaptor<ArbitrationCallback> callbackCaptor = ArgumentCaptor.forClass(ArbitrationCallback.class);
        verify(arbitrator).setArbitrationListener(callbackCaptor.capture());
        verify(arbitrator).scheduleArbitration(false);
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
        }).when(arbitrator).scheduleArbitration(false);

        DiscoveryResult result = subject.discover();

        verify(arbitrator).scheduleArbitration(eq(false));
        assertNotNull(result);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testSecondLookupThrows() throws Exception {
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
        }).when(arbitrator).scheduleArbitration(false);

        DiscoveryResult result = subject.discover();

        verify(arbitrator).scheduleArbitration(eq(false));
        assertNotNull(result);
        subject.discover();
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
        subject.setDiscoveryQos(discoveryQos);

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        subject.buildProxy(testProxy.class, testParticipantId);
        verify(proxyBuilderFactory).get(Mockito.<Set<String>> any(), eq(testProxy.class));
        verify(proxyBuilder).build(Mockito.<ArbitrationResult> any());

        Field discoveryQosField = GuidedProxyBuilder.class.getDeclaredField("discoveryQos");
        discoveryQosField.setAccessible(true);
        DiscoveryQos internalDiscoveryQos = (DiscoveryQos) discoveryQosField.get(subject);
        assert (internalDiscoveryQos.getDiscoveryTimeoutMs() != DiscoveryQos.NO_VALUE);
        assert (internalDiscoveryQos.getRetryIntervalMs() != DiscoveryQos.NO_VALUE);
    }

    private void buildNoneSetup() throws Exception {
        setup();
        String partitipantId = "partitipantId";
        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(partitipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);
        subject.buildNone();
    }

    @Test
    public void buildNoneDecrementsReferenceCountOfAssosiatedRoutingEntries() throws Exception {
        setup();
        String participantId1 = "participantId1";
        String participantId2 = "participantId2";
        String expectedParticipantId1 = participantId1;
        String expectedParticipantId2 = participantId2;
        DiscoveryQos discoveryQos = new DiscoveryQos();
        subject.setDiscoveryQos(discoveryQos);

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry1.setParticipantId(participantId1);
        mockedDiscoveryEntry1.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo(mockedDiscoveryEntry1);
        mockedDiscoveryEntry2.setParticipantId(participantId2);

        mockSuccessfulDiscovery(mockedDiscoveryEntry1, mockedDiscoveryEntry2);

        subject.buildNone();
        verify(proxyBuilderFactory, never()).get(anyString(), any());
        verify(proxyBuilder, never()).build(Mockito.<ArbitrationResult> any());
        verify(messageRouter).removeNextHop(eq(expectedParticipantId1));
        verify(messageRouter).removeNextHop(eq(expectedParticipantId2));
    }

    private void assertNoThrow(ThrowingRunnable r) {
        try {
            r.run();
        } catch (Throwable e) {
            fail("Caught unexpected excetion" + e);
        }

    }

    @Test(expected = JoynrIllegalStateException.class)
    public void discoverThrowsAfterBuildNone() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        subject.discover();
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void setGbidsThrowsAfterBuildNone() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        String[] gbids = new String[]{};
        subject.setGbids(gbids);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void setDiscoveryQosThrowsAfterBuildNone() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        DiscoveryQos discoveryQos = new DiscoveryQos();
        subject.setDiscoveryQos(discoveryQos);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void setMessagingQosThrowsAfterBuildNone() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        MessagingQos messagingQos = new MessagingQos();
        subject.setMessagingQos(messagingQos);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void setStatelessAsyncCallbackUseCaseThrowsAfterBuildNone() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        String statelessAsyncCallbackUseCase = "testUseCase";
        subject.setStatelessAsyncCallbackUseCase(statelessAsyncCallbackUseCase);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void buildProxyThrowsAfterBuildNone() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        String participantId = "participantId";
        subject.buildProxy(testProxy.class, participantId);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void buildNoneThrowsAfterBuildProxy() throws Exception {
        assertNoThrow(() -> {
            setup();
            subject.setDiscoveryQos(new DiscoveryQos());
            String participantId = "participantId";
            DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
            mockedDiscoveryEntry.setParticipantId(participantId);
            mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
            mockSuccessfulDiscovery(mockedDiscoveryEntry);
            subject.buildProxy(testProxy.class, participantId);
        });
        subject.buildNone();
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void buildNoneThrowsOnSecondCall() throws Exception {
        assertNoThrow(() -> buildNoneSetup());
        subject.buildNone();
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void buildNoneThrowsDuringLookup() throws Exception {
        assertNoThrow(() -> {
            setup();
            subject.discoverAsync();
        });
        subject.buildNone();
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void testBuildThrowsOnSecondCall() throws Exception {
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
        subject.buildProxy(testProxy.class, testParticipantId);
    }

    @Test
    public void testProxyBuilderIsProperlyCalledWithBuilderMethods() throws Exception {
        setup();
        String testParticipantId = "test";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        MessagingQos messagingQos = new MessagingQos();
        String statelessAsyncCallbackUseCase = "testUseCase";
        String[] gbids = new String[]{ "gbid1", "gbid2" };
        subject.setGbids(gbids);

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

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

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

    private void mockSuccessfulDiscovery(DiscoveryEntryWithMetaInfo... mockedDiscoveryEntries) throws NoSuchFieldException,
                                                                                               IllegalAccessException {

        Set<DiscoveryEntryWithMetaInfo> mockedSelectedDiscoveryEntries = new HashSet<>(Arrays.asList(mockedDiscoveryEntries));
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
        subject.setDiscoveryQos(discoveryQos);

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(new Version(500, 600));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        subject.buildProxy(testProxy.class, testParticipantId);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuildProxyThrowsOnUnavailableParticipantId() throws Exception {
        setup();
        String testParticipantId = "test";
        DiscoveryQos discoveryQos = new DiscoveryQos();
        subject.setDiscoveryQos(discoveryQos);

        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        mockedDiscoveryEntry.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));
        mockSuccessfulDiscovery(mockedDiscoveryEntry);

        subject.buildProxy(testProxy.class, "unavailableParticipantId");
    }

    @Test
    public void testDiscoverMethodPreservesOtherProviderParticipantIdsInArbitrationResult() throws Exception {
        setup();
        final String participantId1 = "participantId1";
        final String participantId2 = "participantId2";
        final String participantId3 = "participantId3";

        final String otherParticipantId4 = "otherParticipantId4";
        final String otherParticipantId5 = "otherParticipantId5";

        // selectedDiscoveryEntries
        DiscoveryEntryWithMetaInfo selectedDiscoveryEntry1 = new DiscoveryEntryWithMetaInfo();
        selectedDiscoveryEntry1.setParticipantId(participantId1);
        selectedDiscoveryEntry1.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));

        DiscoveryEntryWithMetaInfo selectedDiscoveryEntry2 = new DiscoveryEntryWithMetaInfo();
        selectedDiscoveryEntry2.setParticipantId(participantId2);
        selectedDiscoveryEntry2.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));

        DiscoveryEntryWithMetaInfo selectedDiscoveryEntry3 = new DiscoveryEntryWithMetaInfo();
        selectedDiscoveryEntry3.setParticipantId(participantId3);
        selectedDiscoveryEntry3.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));

        Set<DiscoveryEntryWithMetaInfo> selectedDiscoveryEntries = new HashSet<>(Arrays.asList(selectedDiscoveryEntry1,
                                                                                               selectedDiscoveryEntry2,
                                                                                               selectedDiscoveryEntry3));

        // otherDiscoveryEntries
        DiscoveryEntryWithMetaInfo otherDiscoveryEntry4 = new DiscoveryEntryWithMetaInfo();
        otherDiscoveryEntry4.setParticipantId(otherParticipantId4);
        otherDiscoveryEntry4.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));

        DiscoveryEntryWithMetaInfo otherDiscoveryEntry5 = new DiscoveryEntryWithMetaInfo();
        otherDiscoveryEntry5.setParticipantId(otherParticipantId5);
        otherDiscoveryEntry5.setProviderVersion(VersionUtil.getVersionFromAnnotation(testProxy.class));

        Set<DiscoveryEntryWithMetaInfo> otherDiscoveryEntries = new HashSet<>(Arrays.asList(otherDiscoveryEntry4,
                                                                                            otherDiscoveryEntry5));

        ArbitrationResult arbitrationResult = new ArbitrationResult(selectedDiscoveryEntries, otherDiscoveryEntries);

        DiscoveryQos discoveryQos = new DiscoveryQos();
        subject.setDiscoveryQos(discoveryQos);

        subject.discoverAsync();

        ArgumentCaptor<ArbitrationCallback> callbackCaptor = ArgumentCaptor.forClass(ArbitrationCallback.class);
        verify(arbitrator).setArbitrationListener(callbackCaptor.capture());

        callbackCaptor.getValue().onSuccess(arbitrationResult);

        subject.buildProxy(testProxy.class, participantId1);

        ArgumentCaptor<ArbitrationResult> arbitrationResultCaptor = ArgumentCaptor.forClass(ArbitrationResult.class);

        verify(proxyBuilder).build(arbitrationResultCaptor.capture());

        assertEquals(1, arbitrationResultCaptor.getValue().getDiscoveryEntries().size());
        assertTrue(arbitrationResultCaptor.getValue().getDiscoveryEntries().contains(selectedDiscoveryEntry1));

        assertEquals(4, arbitrationResultCaptor.getValue().getOtherDiscoveryEntries().size());
        assertTrue(arbitrationResultCaptor.getValue().getOtherDiscoveryEntries().contains(otherDiscoveryEntry4));
        assertTrue(arbitrationResultCaptor.getValue().getOtherDiscoveryEntries().contains(otherDiscoveryEntry5));
        assertTrue(arbitrationResultCaptor.getValue().getOtherDiscoveryEntries().contains(selectedDiscoveryEntry2));
        assertTrue(arbitrationResultCaptor.getValue().getOtherDiscoveryEntries().contains(selectedDiscoveryEntry3));

        // check that the original arbitrationResult still contains the original values of selectedDiscoveryEntries, 3 entries
        assertEquals(3, arbitrationResult.getDiscoveryEntries().size());
        assertTrue(arbitrationResult.getDiscoveryEntries().contains(selectedDiscoveryEntry1));
        assertTrue(arbitrationResult.getDiscoveryEntries().contains(selectedDiscoveryEntry2));
        assertTrue(arbitrationResult.getDiscoveryEntries().contains(selectedDiscoveryEntry3));

        // check that the original arbitrationResult still contains the original values of otherDiscoveryEntries, 2 entries
        assertEquals(2, arbitrationResult.getOtherDiscoveryEntries().size());
        assertTrue(arbitrationResult.getOtherDiscoveryEntries().contains(otherDiscoveryEntry4));
        assertTrue(arbitrationResult.getOtherDiscoveryEntries().contains(otherDiscoveryEntry5));
    }

}
