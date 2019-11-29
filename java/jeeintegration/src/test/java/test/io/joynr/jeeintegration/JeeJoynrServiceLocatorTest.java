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
package test.io.joynr.jeeintegration;

import static junit.framework.TestCase.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import javax.enterprise.inject.spi.Bean;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.MessageIdCallback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.jeeintegration.servicelocator.MyService;
import joynr.jeeintegration.servicelocator.MyServiceProxy;
import joynr.jeeintegration.servicelocator.MyServiceStatelessAsync;
import joynr.jeeintegration.servicelocator.MyServiceSync;

import test.io.joynr.jeeintegration.servicelocator.MyServiceCallbackHandler;

/**
 * Unit tests for {@link JeeJoynrServiceLocator}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeJoynrServiceLocatorTest {

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
        when(myJoynrProxy.callMe("one")).thenReturn("two");
        when(proxyBuilderSync.setMessagingQos(Mockito.any())).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.setStatelessAsyncCallbackUseCase(Mockito.anyString())).thenReturn(proxyBuilderSync);
        when(proxyBuilderSync.build()).thenReturn(myJoynrProxy);
        when(joynrRuntime.getProxyBuilder(new HashSet<String>(Arrays.asList("local")),
                                          MyServiceSync.class)).thenReturn(proxyBuilderSync);
        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
        subject = new JeeJoynrServiceLocator(joynrIntegrationBean);
    }

    @Test
    public void testGet() {
        MyServiceSync result = subject.get(MyServiceSync.class, "local");

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertNotNull(callResult);
        assertEquals("two", callResult);
        verify(myJoynrProxy).callMe("one");
    }

    private void forceJoynrProxyToThrowSpecifiedException(Exception exceptionToThrow) {
        reset(myJoynrProxy);
        when(myJoynrProxy.callMe(anyString())).thenThrow(exceptionToThrow);

        MyServiceSync proxy = subject.get(MyServiceSync.class, "local");

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
        reset(myJoynrProxy);
        doThrow(new ApplicationException(MyService.CallMeWithExceptionErrorEnum.MY_ERROR)).when(myJoynrProxy)
                                                                                          .callMeWithException();

        MyServiceSync proxy = subject.get(MyServiceSync.class, "local");

        proxy.callMeWithException();
    }

    @Test
    public void testGetMultiDomain() {
        Set<String> domains = new HashSet<>(Arrays.asList("one", "two", "three"));
        when(joynrRuntime.getProxyBuilder(domains, MyServiceSync.class)).thenReturn(proxyBuilderSync);

        MyServiceSync result = subject.get(MyServiceSync.class, domains);

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertEquals("two", callResult);
        verify(joynrRuntime).getProxyBuilder(domains, MyServiceSync.class);
    }

    @Test
    public void testGetWithTtl() {
        MyServiceSync result = subject.get(MyServiceSync.class, "local", 10000L);

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertNotNull(callResult);
        assertEquals("two", callResult);
        verify(myJoynrProxy).callMe("one");
        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        verify(proxyBuilderSync).setMessagingQos(messagingQosCaptor.capture());
        MessagingQos messagingQosParam = messagingQosCaptor.getValue();
        assertEquals(10000L, messagingQosParam.getRoundTripTtl_ms());
    }

    @Test
    public void testGetWithMessagingAndDiscoveryQos() {
        MessagingQos messagingQos = new MessagingQos();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        MyServiceSync result = subject.get(MyServiceSync.class, "local", messagingQos, discoveryQos);

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertNotNull(callResult);
        assertEquals("two", callResult);
        verify(myJoynrProxy).callMe("one");
        verify(proxyBuilderSync).setMessagingQos(messagingQos);
        verify(proxyBuilderSync).setDiscoveryQos(discoveryQos);
    }

    // The method findJoynrProxyInterface has been removed
    //    @Test(expected = IllegalArgumentException.class)
    //    public void testGetNoProxyAvailable() {
    //        JoynrRuntime joynrRuntime = mock(JoynrRuntime.class);
    //        JoynrIntegrationBean joynrIntegrationBean = mock(JoynrIntegrationBean.class);
    //        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
    //        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(joynrIntegrationBean);
    //
    //        subject.get(MyInvalidServiceSync.class, "local");
    //    }

    @Test(expected = IllegalStateException.class)
    public void testGetNoRuntime() {
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(mock(JoynrIntegrationBean.class));
        subject.get(MyServiceSync.class, "local");
    }

    @Test
    public void testGetWithStatelessCallbackHandler() {
        when(joynrRuntime.getProxyBuilder(new HashSet<String>(Arrays.asList("local")),
                                          MyServiceStatelessAsync.class)).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.setMessagingQos(Mockito.any())).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.build()).thenReturn(myJoynrProxy);
        doAnswer(invocation -> {
            invocation.getArgumentAt(1, MessageIdCallback.class).accept("messageId");
            return null;
        }).when(myJoynrProxy).callMe(eq("one"), any(MessageIdCallback.class));
        @SuppressWarnings("rawtypes")
        Bean mockBean = mock(Bean.class);
        when(mockBean.getBeanClass()).thenReturn(MyServiceCallbackHandler.class);
        MyServiceStatelessAsync result = subject.builder(MyServiceStatelessAsync.class, "local")
                                                .withUseCase(MyServiceCallbackHandler.USE_CASE)
                                                .build();

        assertNotNull(result);

        verify(joynrRuntime).getProxyBuilder(eq(new HashSet<>(Arrays.asList("local"))),
                                             eq(MyServiceStatelessAsync.class));
        verify(proxyBuilderStatelessAsync).setStatelessAsyncCallbackUseCase(eq(MyServiceCallbackHandler.USE_CASE));
        verify(proxyBuilderStatelessAsync).build();

        Boolean[] resultContainer = new Boolean[]{ Boolean.FALSE };
        result.callMe("one", messageId -> {
            assertEquals("messageId", messageId);
            resultContainer[0] = Boolean.TRUE;
        });
        assertTrue(resultContainer[0]);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetWithSyncAndUseCaseFails() {
        subject.builder(MyServiceSync.class, "local").withUseCase("useCase").build();
        fail("Should not be able to build a service proxy with a use case and a non @StatelessAsync service interface");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetStatelessAsyncWithoutUseCaseFails() {
        when(joynrRuntime.getProxyBuilder(new HashSet<String>(Arrays.asList("local")),
                                          MyServiceStatelessAsync.class)).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.setMessagingQos(Mockito.any())).thenReturn(proxyBuilderStatelessAsync);
        when(proxyBuilderStatelessAsync.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilderStatelessAsync);
        subject.builder(MyServiceStatelessAsync.class, "local").build();
        fail("Should not be able to build stateless async proxy without a use case specified.");
    }

}
