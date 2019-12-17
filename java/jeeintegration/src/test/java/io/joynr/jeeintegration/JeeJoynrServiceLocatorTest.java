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
package io.joynr.jeeintegration;

import static junit.framework.TestCase.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anySetOf;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.jeeintegration.servicelocator.MyServiceCallbackHandler;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.GuidedProxyBuilder;
import io.joynr.proxy.MessageIdCallback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.JoynrRuntime;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.jeeintegration.servicelocator.MyService;
import joynr.jeeintegration.servicelocator.MyServiceProxy;
import joynr.jeeintegration.servicelocator.MyServiceStatelessAsync;
import joynr.jeeintegration.servicelocator.MyServiceSync;

/**
 * Unit tests for {@link JeeJoynrServiceLocator}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeJoynrServiceLocatorTest {

    @Captor
    private ArgumentCaptor<ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> callbackSyncCaptor;

    @Mock
    private MyServiceProxy myJoynrProxy;

    @Mock
    private ProxyBuilder<MyServiceSync> proxyBuilderSync;
    @Mock
    private ProxyBuilder<MyServiceStatelessAsync> proxyBuilderStatelessAsync;

    @Mock
    private JoynrRuntime joynrRuntime;

    @Mock
    private JoynrIntegrationBean joynrIntegrationBean;

    private JeeJoynrServiceLocator subject;

    @Before
    public void setupSubject() {
        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
        subject = new JeeJoynrServiceLocator(joynrIntegrationBean);
    }

    private void setupSyncInterface() {
        when(joynrRuntime.getProxyBuilder(new HashSet<String>(Arrays.asList("local")),
                                          MyServiceSync.class)).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.setMessagingQos(Mockito.any())).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.setGbids(Mockito.<String[]> any())).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.build()).thenReturn(myJoynrProxy);

        when(myJoynrProxy.callMe("one")).thenReturn("two");
    }

    private void setupStatelessAsyncInterface() {
        when(joynrRuntime.getProxyBuilder(new HashSet<String>(Arrays.asList("local")),
                                          MyServiceStatelessAsync.class)).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.setMessagingQos(Mockito.any())).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.build()).thenReturn(myJoynrProxy);

        doAnswer(invocation -> {
            invocation.getArgumentAt(1, MessageIdCallback.class).accept("messageId");
            return null;
        }).when(myJoynrProxy).callMe(eq("one"), any(MessageIdCallback.class));
    }

    private void testSyncProxyCall(MyServiceSync proxy) {
        assertNotNull(proxy);

        String callResult = proxy.callMe("one");

        assertNotNull(callResult);
        assertEquals("two", callResult);
        verify(myJoynrProxy).callMe("one");
    }

    private void testStatelessAsyncProxyCall(MyServiceStatelessAsync proxy) {
        assertNotNull(proxy);

        verify(proxyBuilderStatelessAsync).setStatelessAsyncCallbackUseCase(eq(MyServiceCallbackHandler.USE_CASE));

        Boolean[] resultContainer = new Boolean[]{ Boolean.FALSE };
        proxy.callMe("one", messageId -> {
            assertEquals("messageId", messageId);
            resultContainer[0] = Boolean.TRUE;
        });
        assertTrue(resultContainer[0]);
    }

    @Test
    public void testGet() {
        setupSyncInterface();

        @SuppressWarnings("deprecation")
        MyServiceSync result = subject.get(MyServiceSync.class, "local");

        testSyncProxyCall(result);
    }

    @Test
    public void testGetWithTtl() {
        setupSyncInterface();

        MessagingQos expectedMessagingQos = new MessagingQos(10000);
        @SuppressWarnings("deprecation")
        MyServiceSync result = subject.get(MyServiceSync.class, "local", expectedMessagingQos.getRoundTripTtl_ms());

        verify(proxyBuilderSync).setMessagingQos(expectedMessagingQos);

        testSyncProxyCall(result);

        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        verify(proxyBuilderSync).setMessagingQos(messagingQosCaptor.capture());
        MessagingQos messagingQosParam = messagingQosCaptor.getValue();
        assertEquals(10000L, messagingQosParam.getRoundTripTtl_ms());
    }

    @Test
    public void testGet_multiDomain() {
        setupSyncInterface();

        Set<String> domains = new HashSet<>(Arrays.asList("one", "two", "three"));
        when(joynrRuntime.getProxyBuilder(domains, MyServiceSync.class)).thenReturn(proxyBuilderSync);

        @SuppressWarnings("deprecation")
        MyServiceSync result = subject.get(MyServiceSync.class, domains);

        verify(joynrRuntime).getProxyBuilder(domains, MyServiceSync.class);

        testSyncProxyCall(result);
    }

    @Test
    public void testGet_multiDomain_withTtl() {
        setupSyncInterface();

        Set<String> domains = new HashSet<>(Arrays.asList("one", "two", "three"));
        when(joynrRuntime.getProxyBuilder(domains, MyServiceSync.class)).thenReturn(proxyBuilderSync);

        @SuppressWarnings("deprecation")
        MyServiceSync result = subject.get(MyServiceSync.class, domains, 10000L);

        verify(joynrRuntime).getProxyBuilder(domains, MyServiceSync.class);

        testSyncProxyCall(result);
    }

    @Test
    public void testGet_withMessagingAndDiscoveryQos() {
        setupSyncInterface();

        MessagingQos messagingQos = new MessagingQos();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        @SuppressWarnings("deprecation")
        MyServiceSync result = subject.get(MyServiceSync.class, "local", messagingQos, discoveryQos);

        assertNotNull(result);

        verify(proxyBuilderSync).setMessagingQos(messagingQos);
        verify(proxyBuilderSync).setDiscoveryQos(discoveryQos);

        testSyncProxyCall(result);
    }

    @Test
    public void testGet_multiDomain_withMessagingAndDiscoveryQos() {
        setupSyncInterface();

        Set<String> domains = new HashSet<>(Arrays.asList("one", "two", "three"));
        when(joynrRuntime.getProxyBuilder(domains, MyServiceSync.class)).thenReturn(proxyBuilderSync);
        MessagingQos messagingQos = new MessagingQos();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        @SuppressWarnings("deprecation")
        MyServiceSync result = subject.get(MyServiceSync.class, domains, messagingQos, discoveryQos);

        verify(joynrRuntime).getProxyBuilder(domains, MyServiceSync.class);

        verify(proxyBuilderSync).setMessagingQos(messagingQos);
        verify(proxyBuilderSync).setDiscoveryQos(discoveryQos);

        testSyncProxyCall(result);
    }

    @Test
    public void testGet_multiDomain_withMessagingAndDiscoveryQosAndUseCase() {
        setupStatelessAsyncInterface();

        Set<String> domains = new HashSet<>(Arrays.asList("one", "two", "three"));
        when(joynrRuntime.getProxyBuilder(domains,
                                          MyServiceStatelessAsync.class)).thenReturn(proxyBuilderStatelessAsync);
        MessagingQos messagingQos = new MessagingQos();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        @SuppressWarnings("deprecation")
        MyServiceStatelessAsync result = subject.get(MyServiceStatelessAsync.class,
                                                     domains,
                                                     messagingQos,
                                                     discoveryQos,
                                                     "useCase");

        verify(joynrRuntime).getProxyBuilder(eq(domains), eq(MyServiceStatelessAsync.class));

        testStatelessAsyncProxyCall(result);
        verify(proxyBuilderStatelessAsync).build();
    }

    @SuppressWarnings("deprecation")
    @Test(expected = IllegalArgumentException.class)
    public void testGet_fails_withSyncInterfaceAndUseCase() {
        setupSyncInterface();

        Set<String> domains = new HashSet<>(Arrays.asList("local"));
        subject.get(MyServiceSync.class, domains, new MessagingQos(), new DiscoveryQos(), "useCase");

        fail("Should not be able to build a service proxy with a use case and a non @StatelessAsync service interface");
    }

    @SuppressWarnings("deprecation")
    @Test(expected = IllegalArgumentException.class)
    public void testGet_fails_withStatelessAsyncInterfaceWithoutUseCase() {
        setupStatelessAsyncInterface();

        Set<String> domains = new HashSet<>(Arrays.asList("local"));
        subject.get(MyServiceStatelessAsync.class, domains, new MessagingQos(), new DiscoveryQos(), (String) null);

        fail("Should not be able to build stateless async proxy without a use case specified.");
    }

    @SuppressWarnings("deprecation")
    @Test(expected = IllegalStateException.class)
    public void testGet_fails_noRuntime() {
        when(joynrIntegrationBean.getRuntime()).thenReturn(null);
        subject.get(MyServiceSync.class, "local");
    }

    @Test
    public void testGetGuidedProxyBuilder() {
        Set<String> domains = new HashSet<>();
        domains.add("testDomain");
        Set<String> expectedDomains = new HashSet<>(domains);

        when(joynrRuntime.getGuidedProxyBuilder(anySetOf(String.class),
                                                any(Class.class))).thenReturn(mock(GuidedProxyBuilder.class));

        assertNotNull(subject.getGuidedProxyBuilder(MyServiceSync.class, domains));

        verify(joynrIntegrationBean).getRuntime();
        verify(joynrRuntime).getGuidedProxyBuilder(expectedDomains, MyServiceSync.class);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuilder_fails_withSyncInterfaceAndUseCase() {
        setupSyncInterface();

        subject.builder(MyServiceSync.class, "local").withUseCase("useCase").build();
        fail("Should not be able to build a service proxy with a use case and a non @StatelessAsync service interface");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuilder_withCallback_fails_withSyncInterfaceAndUseCase() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        setupSyncInterface();

        ProxyCreatedCallback<MyServiceSync> callback = new ProxyCreatedCallback<MyServiceSync>() {
            @Override
            public void onProxyCreationFinished(MyServiceSync result) {
                fail("proxy creation succeeded");
                semaphore.release();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("callback: onProxyCreationError called: " + error);
                semaphore.release();
            }
        };

        subject.builder(MyServiceSync.class, "local").withCallback(callback).withUseCase("useCase").build();
        fail("Should not be able to build a service proxy with a use case and a non @StatelessAsync service interface");

        assertTrue(semaphore.tryAcquire(1, TimeUnit.SECONDS));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuilder_withFuture_fails_withSyncInterfaceAndUseCase() throws Exception {
        setupSyncInterface();
        CountDownLatch countDownLatch = new CountDownLatch(1);
        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local")
                                                         .useFuture()
                                                         .withUseCase("useCase")
                                                         .build();
        fail("Should not be able to build a service proxy with a use case and a non @StatelessAsync service interface");
        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null && error == null) {
                fail("future: proxy creation succeeded");
            } else if (error != null) {
                fail("future: error: " + error);
            }
            countDownLatch.countDown();
        });
        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuilder_fails_withStatelessAsyncInterfaceWithoutUseCase() {
        setupStatelessAsyncInterface();

        subject.builder(MyServiceStatelessAsync.class, "local").build();
        fail("Should not be able to build stateless async proxy without a use case specified.");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuilder_withCallback_fails_withStatelessAsyncInterfaceWithoutUseCase() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        setupStatelessAsyncInterface();

        ProxyCreatedCallback<MyServiceStatelessAsync> callback = new ProxyCreatedCallback<MyServiceStatelessAsync>() {
            @Override
            public void onProxyCreationFinished(MyServiceStatelessAsync result) {
                fail("proxy creation succeeded");
                semaphore.release();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("callback: onProxyCreationError called: " + error);
                semaphore.release();
            }
        };

        subject.builder(MyServiceStatelessAsync.class, "local").withCallback(callback).build();
        fail("Should not be able to build stateless async proxy without a use case specified.");

        assertTrue(semaphore.tryAcquire(1, TimeUnit.SECONDS));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBuilder_withFuture_fails_withStatelessAsyncInterfaceWithoutUseCase() throws Exception {
        setupStatelessAsyncInterface();

        CountDownLatch countDownLatch = new CountDownLatch(1);
        CompletableFuture<MyServiceStatelessAsync> future = subject.builder(MyServiceStatelessAsync.class, "local")
                                                                   .useFuture()
                                                                   .build();
        fail("Should not be able to build stateless async proxy without a use case specified.");
        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null && error == null) {
                fail("future: proxy creation succeeded");
            } else if (error != null) {
                fail("future: error: " + error);
            }
            countDownLatch.countDown();
        });
        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }

    @Test(expected = IllegalStateException.class)
    public void testBuilder_fails_noRuntime() {
        when(joynrIntegrationBean.getRuntime()).thenReturn(null);
        subject.builder(MyServiceSync.class, "local").build();
        fail("Should not be able to build a proxy without runtime.");
    }

    @Test(expected = IllegalStateException.class)
    public void testBuilder_withCallback_fails_noRuntime() throws Exception {
        Semaphore semaphore = new Semaphore(0);
        when(joynrIntegrationBean.getRuntime()).thenReturn(null);

        ProxyCreatedCallback<MyServiceSync> callback = new ProxyCreatedCallback<MyServiceSync>() {
            @Override
            public void onProxyCreationFinished(MyServiceSync result) {
                fail("callback: proxy creation succeeded");
                semaphore.release();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("callback: onProxyCreationError called: " + error);
                semaphore.release();
            }
        };

        subject.builder(MyServiceSync.class, "local").withCallback(callback).build();
        fail("Should not be able to build a proxy without runtime.");

        assertTrue(semaphore.tryAcquire(1, TimeUnit.SECONDS));
    }

    @Test(expected = IllegalStateException.class)
    public void testBuilder_withFuture_fails_noRuntime() throws Exception {
        when(joynrIntegrationBean.getRuntime()).thenReturn(null);

        CountDownLatch countDownLatch = new CountDownLatch(1);
        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local").useFuture().build();
        fail("Should not be able to build a proxy without runtime.");
        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null && error == null) {
                fail("future: proxy creation succeeded");
            } else if (error != null) {
                fail("future: error: " + error);
            }
            countDownLatch.countDown();
        });
        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }

    private void forceJoynrProxyToThrowSpecifiedException(Exception exceptionToThrow) {
        setupSyncInterface();

        reset(myJoynrProxy);
        when(myJoynrProxy.callMe(anyString())).thenThrow(exceptionToThrow);

        MyServiceSync proxy = subject.builder(MyServiceSync.class, "local").build();

        proxy.callMe("one");
    }

    @Test(expected = ProviderRuntimeException.class)
    public void testProxyUnwrapsProviderRuntimeException() {
        forceJoynrProxyToThrowSpecifiedException(new ProviderRuntimeException("test"));
    }

    @Test(expected = JoynrCommunicationException.class)
    public void testProxyUnwrapsJoynrCommunicationException() {
        forceJoynrProxyToThrowSpecifiedException(new JoynrCommunicationException("test"));
    }

    @Test(expected = JoynrTimeoutException.class)
    public void testProxyUnwrapsJoynrTimeoutException() {
        forceJoynrProxyToThrowSpecifiedException(new JoynrTimeoutException(42));
    }

    @Test(expected = JoynrRuntimeException.class)
    public void testProxyUnwrapsJoynrRuntimeException() {
        forceJoynrProxyToThrowSpecifiedException(new JoynrRuntimeException("test"));
    }

    @Test(expected = ApplicationException.class)
    public void testProxyUnwrapsApplicationException() throws Exception {
        setupSyncInterface();

        doThrow(new ApplicationException(MyService.CallMeWithExceptionErrorEnum.MY_ERROR)).when(myJoynrProxy)
                                                                                          .callMeWithException();

        MyServiceSync proxy = subject.builder(MyServiceSync.class, "local").build();

        proxy.callMeWithException();
    }

    @Test
    public void testBuilder() {
        setupSyncInterface();

        MyServiceSync result = subject.builder(MyServiceSync.class, "local").build();

        verify(joynrRuntime).getProxyBuilder(eq(new HashSet<>(Arrays.asList("local"))), eq(MyServiceSync.class));

        testSyncProxyCall(result);
    }

    @Test
    public void testBuilder_multiDomain() {
        setupSyncInterface();
        Set<String> domains = new HashSet<>(Arrays.asList("one", "two", "three"));
        when(joynrRuntime.getProxyBuilder(domains, MyServiceSync.class)).thenReturn(proxyBuilderSync);

        MyServiceSync result = subject.builder(MyServiceSync.class, domains.toArray(new String[domains.size()]))
                                      .build();

        verify(joynrRuntime).getProxyBuilder(eq(domains), eq(MyServiceSync.class));

        testSyncProxyCall(result);
    }

    @Test
    public void testBuilder_withTtl() {
        setupSyncInterface();

        MessagingQos expectedMessagingQos = new MessagingQos(42);
        MyServiceSync result = subject.builder(MyServiceSync.class, "local")
                                      .withTtl(expectedMessagingQos.getRoundTripTtl_ms())
                                      .build();

        verify(joynrRuntime).getProxyBuilder(eq(new HashSet<>(Arrays.asList("local"))), eq(MyServiceSync.class));
        verify(proxyBuilderSync).setMessagingQos(expectedMessagingQos);

        testSyncProxyCall(result);
    }

    @Test
    public void testBuilder_withMessagingQos() {
        setupSyncInterface();

        MessagingQos messagingQos = new MessagingQos(43);
        MessagingQos expectedMessagingQos = new MessagingQos(messagingQos);
        MyServiceSync result = subject.builder(MyServiceSync.class, "local").withMessagingQos(messagingQos).build();

        verify(joynrRuntime).getProxyBuilder(eq(new HashSet<>(Arrays.asList("local"))), eq(MyServiceSync.class));
        verify(proxyBuilderSync).setMessagingQos(expectedMessagingQos);

        testSyncProxyCall(result);
    }

    @Test
    public void testBuilder_withDiscoveryQos() {
        setupSyncInterface();

        DiscoveryQos expectedDiscoveryQos = new DiscoveryQos(42, ArbitrationStrategy.FixedChannel, 32);
        MyServiceSync result = subject.builder(MyServiceSync.class, "local")
                                      .withDiscoveryQos(expectedDiscoveryQos)
                                      .build();

        verify(joynrRuntime).getProxyBuilder(eq(new HashSet<>(Arrays.asList("local"))), eq(MyServiceSync.class));
        verify(proxyBuilderSync).setDiscoveryQos(expectedDiscoveryQos);

        testSyncProxyCall(result);
    }

    @Test
    public void testBuilder_withUseCase() {
        setupStatelessAsyncInterface();

        MyServiceStatelessAsync result = subject.builder(MyServiceStatelessAsync.class, "local")
                                                .withUseCase(MyServiceCallbackHandler.USE_CASE)
                                                .build();

        verify(joynrRuntime).getProxyBuilder(eq(new HashSet<>(Arrays.asList("local"))),
                                             eq(MyServiceStatelessAsync.class));

        testStatelessAsyncProxyCall(result);
        verify(proxyBuilderStatelessAsync).build();
    }

    @Test
    public void testBuilder_withGbids() {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        String[] gbids = new String[]{ "gbid1", "gbid2" };
        subject.builder(MyServiceSync.class, "local").withGbids(gbids).build();
        verify(proxyBuilderSync).setGbids(gbids);
    }

    @Test
    public void testBuilder_withCallback() throws Exception {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        CountDownLatch countDownLatch = new CountDownLatch(1);
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> callback = new ProxyBuilder.ProxyCreatedCallback<MyServiceSync>() {
            @Override
            public void onProxyCreationFinished(MyServiceSync result) {
                countDownLatch.countDown();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("Should never get called.");
                countDownLatch.countDown();
            }
        };

        MyServiceSync result = subject.builder(MyServiceSync.class, "local").withCallback(callback).build();
        assertNotNull(result);

        verify(proxyBuilderSync).build(callbackSyncCaptor.capture());
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> capturedCallback = callbackSyncCaptor.getValue();
        capturedCallback.onProxyCreationFinished(myJoynrProxy);

        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
        verify(proxyBuilderSync, times(0)).build();
    }

    @Test
    public void testBuilder_withCallback_fails() throws Exception {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        CountDownLatch countDownLatch = new CountDownLatch(1);
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> callback = new ProxyBuilder.ProxyCreatedCallback<MyServiceSync>() {
            @Override
            public void onProxyCreationFinished(MyServiceSync result) {
                fail("Should never get called.");
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                countDownLatch.countDown();
            }
        };

        subject.builder(MyServiceSync.class, "local").withCallback(callback).build();

        verify(proxyBuilderSync).build(callbackSyncCaptor.capture());
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> capturedCallback = callbackSyncCaptor.getValue();
        capturedCallback.onProxyCreationError(new JoynrRuntimeException());

        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }

    @Test
    public void testBuilder_withFuture() throws Exception {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        CountDownLatch countDownLatch = new CountDownLatch(1);
        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local").useFuture().build();
        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null && error == null) {
                countDownLatch.countDown();
            }
        });

        verify(proxyBuilderSync).build(callbackSyncCaptor.capture());
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> value = callbackSyncCaptor.getValue();
        value.onProxyCreationFinished(myJoynrProxy);

        countDownLatch.await(100L, TimeUnit.MILLISECONDS);

        MyServiceSync myServiceSync = future.get();
        assertNotNull(myServiceSync);
    }

    @Test
    public void testBuilder_withFuture_fails() throws Exception {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        CountDownLatch countDownLatch = new CountDownLatch(1);
        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local").useFuture().build();
        future.whenCompleteAsync((proxy, error) -> {
            if (error != null && proxy == null) {
                countDownLatch.countDown();
            }
        });

        verify(proxyBuilderSync).build(callbackSyncCaptor.capture());
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> value = callbackSyncCaptor.getValue();
        value.onProxyCreationError(new JoynrRuntimeException("test"));

        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
        try {
            future.get();
            fail("Should never get this far.");
        } catch (ExecutionException e) {
            if (!(e.getCause() instanceof JoynrRuntimeException)) {
                fail("Nested exception not of expected type.");
            }
        }
    }

    @Test
    public void testBuilder_withFutureAndCallback() throws Exception {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        CountDownLatch countDownLatch = new CountDownLatch(2);
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> callback = new ProxyBuilder.ProxyCreatedCallback<MyServiceSync>() {
            @Override
            public void onProxyCreationFinished(MyServiceSync result) {
                countDownLatch.countDown();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                fail("Should never get called");
            }
        };

        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local")
                                                         .withCallback(callback)
                                                         .useFuture()
                                                         .build();

        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null && error == null) {
                countDownLatch.countDown();
            }
        });

        verify(proxyBuilderSync).build(callbackSyncCaptor.capture());
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> capturedCallback = callbackSyncCaptor.getValue();
        capturedCallback.onProxyCreationFinished(myJoynrProxy);

        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }

    @Test
    public void testBuilder_withFutureAndCallback_fails() throws Exception {
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        CountDownLatch countDownLatch = new CountDownLatch(2);
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> callback = new ProxyBuilder.ProxyCreatedCallback<MyServiceSync>() {
            @Override
            public void onProxyCreationFinished(MyServiceSync result) {
                fail("Should never get called");
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                countDownLatch.countDown();
            }
        };

        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local")
                                                         .withCallback(callback)
                                                         .useFuture()
                                                         .build();

        future.whenCompleteAsync((proxy, error) -> {
            if (proxy == null && error != null) {
                countDownLatch.countDown();
            }
        });

        verify(proxyBuilderSync).build(callbackSyncCaptor.capture());
        ProxyBuilder.ProxyCreatedCallback<MyServiceSync> capturedCallback = callbackSyncCaptor.getValue();
        capturedCallback.onProxyCreationError(new JoynrRuntimeException());

        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }

    @Test
    public void testBuilder_withFutureAndGbids() throws Exception {
        CountDownLatch countDownLatch = new CountDownLatch(1);
        setupSyncInterface();

        when(proxyBuilderSync.build(Mockito.<io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback<MyServiceSync>> any())).thenReturn(myJoynrProxy);

        String[] gbids = new String[]{ "gbid1", "gbid2" };
        CompletableFuture<MyServiceSync> future = subject.builder(MyServiceSync.class, "local")
                                                         .useFuture()
                                                         .withGbids(gbids)
                                                         .build();
        verify(proxyBuilderSync).setGbids(gbids);
        future.whenCompleteAsync((proxy, error) -> {
            if (proxy != null && error == null) {
                countDownLatch.countDown();
            }
        });
        countDownLatch.await(100L, TimeUnit.MILLISECONDS);
    }
}
