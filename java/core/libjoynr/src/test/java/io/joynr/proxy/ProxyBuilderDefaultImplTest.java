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
package io.joynr.proxy;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.mockito.Mockito.any;

import java.lang.reflect.Field;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import io.joynr.JoynrVersion;
import io.joynr.arbitration.ArbitrationCallback;
import io.joynr.arbitration.ArbitrationResult;
import io.joynr.arbitration.Arbitrator;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.discovery.LocalDiscoveryAggregator;
import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.MultiDomainNoCompatibleProviderFoundException;
import io.joynr.exceptions.NoCompatibleProviderFoundException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.ShutdownNotifier;
import joynr.types.DiscoveryEntryWithMetaInfo;
import joynr.types.Version;

/**
 * Unit tests for the {@link ProxyBuilderDefaultImpl}.
 */
@RunWith(MockitoJUnitRunner.class)
public class ProxyBuilderDefaultImplTest {

    private static final long MAX_MESSAGE_TTL = 1024L;
    private static final long DEFAULT_DISCOVERY_TIMEOUT_MS = 30000L;
    private static final long DEFAULT_RETRY_INTERVAL_MS = 2000L;

    @JoynrVersion(major = 1, minor = 1)
    private static interface TestInterface {
        String INTERFACE_NAME = "test/interface";
    }

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
    private ShutdownNotifier shutdownNotifier;

    @Captor
    private ArgumentCaptor<ArbitrationCallback> arbitrationCallbackCaptor;

    @Captor
    private ArgumentCaptor<JoynrRuntimeException> exceptionCaptor;

    private ProxyBuilderDefaultImpl<TestInterface> subject;

    @Mock
    private ProxyCreatedCallback<TestInterface> proxyCreatedCallback;

    public void setup(Set<String> domains) throws Exception {
        subject = new ProxyBuilderDefaultImpl<TestInterface>(localDiscoveryAggregator,
                                                             domains,
                                                             TestInterface.class,
                                                             proxyInvocationHandlerFactory,
                                                             shutdownNotifier,
                                                             statelessAsyncCallbackDirectory,
                                                             MAX_MESSAGE_TTL,
                                                             DEFAULT_DISCOVERY_TIMEOUT_MS,
                                                             DEFAULT_RETRY_INTERVAL_MS);
        Field arbitratorField = ProxyBuilderDefaultImpl.class.getDeclaredField("arbitrator");
        arbitratorField.setAccessible(true);
        arbitratorField.set(subject, arbitrator);
        when(proxyInvocationHandlerFactory.create(Mockito.<Set<String>> any(),
                                                  eq(TestInterface.INTERFACE_NAME),
                                                  Mockito.<String> any(),
                                                  Mockito.<DiscoveryQos> any(),
                                                  Mockito.<MessagingQos> any(),
                                                  Mockito.<ShutdownNotifier> any(),
                                                  Mockito.<StatelessAsyncCallback> any())).thenReturn(proxyInvocationHandler);
    }

    @Test
    public void testNoCompatibleProviderPassedToOnError() throws Exception {
        final String domain = "domain1";
        final Set<String> domains = new HashSet<>(Arrays.asList(domain));
        setup(domains);
        final ExecutorService executor = Executors.newSingleThreadExecutor();
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                executor.submit(new Callable<Void>() {
                    @Override
                    public Void call() throws Exception {
                        Thread.sleep(10L);
                        verify(arbitrator).setArbitrationListener(arbitrationCallbackCaptor.capture());
                        ArbitrationCallback callback = arbitrationCallbackCaptor.getValue();
                        Set<Version> discoveredVersions = new HashSet<>(Arrays.asList(new Version(100, 100)));
                        callback.onError(new NoCompatibleProviderFoundException(TestInterface.INTERFACE_NAME,
                                                                                new Version(1, 1),
                                                                                domain,
                                                                                discoveredVersions));
                        return null;
                    }
                });
                return null;
            }
        }).when(arbitrator).scheduleArbitration();
        subject.build(proxyCreatedCallback);
        executor.shutdown();
        executor.awaitTermination(100L, TimeUnit.MILLISECONDS);
        verify(proxyCreatedCallback).onProxyCreationError(exceptionCaptor.capture());
        JoynrRuntimeException capturedException = exceptionCaptor.getValue();
        assertTrue(capturedException instanceof NoCompatibleProviderFoundException);
    }

    @Test
    public void testMultiDomainNoCompatibleProviderFoundSetOnInvocationHandler() throws Exception {
        final Set<String> domains = new HashSet<>(Arrays.asList("domain-1", "domain-2"));
        setup(domains);
        final ExecutorService executor = Executors.newSingleThreadExecutor();
        doAnswer(new Answer<Void>() {
            @Override
            public Void answer(InvocationOnMock invocation) throws Throwable {
                executor.submit(new Callable<Void>() {
                    @Override
                    public Void call() throws Exception {
                        Thread.sleep(10L);
                        verify(arbitrator).setArbitrationListener(arbitrationCallbackCaptor.capture());
                        ArbitrationCallback callback = arbitrationCallbackCaptor.getValue();
                        Map<String, Set<Version>> versionsByDomain = new HashMap<>();
                        HashSet<Version> discoveredVersions = new HashSet<>(Arrays.asList(new Version(100, 100)));
                        Map<String, NoCompatibleProviderFoundException> exceptionsByDomain = new HashMap<>();
                        for (String domain : domains) {
                            versionsByDomain.put(domain, discoveredVersions);
                            exceptionsByDomain.put(domain,
                                                   new NoCompatibleProviderFoundException("interfaceName",
                                                                                          new Version(1, 1),
                                                                                          domain,
                                                                                          discoveredVersions));
                        }
                        callback.onError(new MultiDomainNoCompatibleProviderFoundException(exceptionsByDomain));
                        return null;
                    }
                });
                return null;
            }
        }).when(arbitrator).scheduleArbitration();
        subject.build(proxyCreatedCallback);
        executor.shutdown();
        executor.awaitTermination(100L, TimeUnit.MILLISECONDS);
        verify(proxyCreatedCallback).onProxyCreationError(exceptionCaptor.capture());
        JoynrRuntimeException capturedException = exceptionCaptor.getValue();
        assertTrue(capturedException instanceof MultiDomainNoCompatibleProviderFoundException);
    }

    @Test(expected = JoynrIllegalStateException.class)
    public void cantBuildForMissingStatelessAsyncCallback() throws Exception {
        setup(new HashSet<String>(Arrays.asList("domain")));
        subject.setStatelessAsyncCallbackUseCase("invalid");
        subject.build();
    }

    @Test
    public void fetchesStatelessAsyncCallbackIfUseCaseSet() throws Exception {
        setup(new HashSet<String>(Arrays.asList("domain")));
        final String useCase = "useCase";
        StatelessAsyncCallback callbackMock = mock(StatelessAsyncCallback.class);
        when(statelessAsyncCallbackDirectory.get(eq(useCase))).thenReturn(callbackMock);
        subject.setStatelessAsyncCallbackUseCase(useCase);
        subject.build();
        verify(statelessAsyncCallbackDirectory).get(eq(useCase));
    }

    @Test
    public void setMessagingQosSetsCorrectValue() throws Exception {
        setup(new HashSet<String>(Arrays.asList("domain")));
        MessagingQos messagingQos = new MessagingQos();
        subject.setMessagingQos(messagingQos);

        Field dMessagingQosField = subject.getClass().getDeclaredField("messagingQos");
        dMessagingQosField.setAccessible(true);
        MessagingQos newMessagingQos = (MessagingQos) dMessagingQosField.get(subject);

        assertEquals(messagingQos, newMessagingQos);
    }

    @Test
    public void buildMethodForGuidedBuilder() throws Exception {
        setup(new HashSet<String>(Arrays.asList("domain")));
        String testParticipantId = "test";
        DiscoveryEntryWithMetaInfo mockedDiscoveryEntry = new DiscoveryEntryWithMetaInfo();
        mockedDiscoveryEntry.setParticipantId(testParticipantId);
        ArbitrationResult mockedArbitrationResult = new ArbitrationResult(mockedDiscoveryEntry);
        subject.build(mockedArbitrationResult);
        verify(proxyInvocationHandlerFactory).create(any(), any(), any(), any(), any(), any(), any());
        verify(proxyInvocationHandler).registerProxy(any());
        verify(proxyInvocationHandler).createConnector(any());
    }

}
