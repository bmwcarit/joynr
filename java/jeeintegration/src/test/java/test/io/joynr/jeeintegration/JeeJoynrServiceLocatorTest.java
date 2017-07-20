package test.io.joynr.jeeintegration;

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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.Set;

import com.google.common.collect.Sets;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.exceptions.JoynrTimeoutException;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.jeeintegration.servicelocator.MyService;
import joynr.jeeintegration.servicelocator.MyServiceProxy;
import joynr.jeeintegration.servicelocator.MyServiceSync;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import test.io.joynr.jeeintegration.servicelocator.MyInvalidServiceSync;

/**
 * Unit tests for {@link JeeJoynrServiceLocator}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeJoynrServiceLocatorTest {

    @Mock
    private MyServiceProxy myJoynrProxy;

    @Mock
    private ProxyBuilder<MyServiceProxy> proxyBuilder;

    @Mock
    private JoynrRuntime joynrRuntime;

    @Mock
    private JoynrIntegrationBean joynrIntegrationBean;

    private JeeJoynrServiceLocator subject;

    @Before
    public void setupSubject() {
        when(myJoynrProxy.callMe("one")).thenReturn("two");
        when(proxyBuilder.setMessagingQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.build()).thenReturn(myJoynrProxy);
        when(joynrRuntime.getProxyBuilder(Sets.newHashSet("local"), MyServiceProxy.class)).thenReturn(proxyBuilder);
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

    private void forceJoynrProxyToThrowSpecifiedException(Exception exceptionToThrow){
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
        Set<String> domains = Sets.newHashSet("one", "two", "three");
        when(joynrRuntime.getProxyBuilder(domains, MyServiceProxy.class)).thenReturn(proxyBuilder);

        MyServiceSync result = subject.get(MyServiceSync.class, domains);

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertEquals("two", callResult);
        verify(joynrRuntime).getProxyBuilder(domains, MyServiceProxy.class);
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
        verify(proxyBuilder).setMessagingQos(messagingQosCaptor.capture());
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
        verify(proxyBuilder).setMessagingQos(messagingQos);
        verify(proxyBuilder).setDiscoveryQos(discoveryQos);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testGetNoProxyAvailable() {
        JoynrRuntime joynrRuntime = mock(JoynrRuntime.class);
        JoynrIntegrationBean joynrIntegrationBean = mock(JoynrIntegrationBean.class);
        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(joynrIntegrationBean);

        subject.get(MyInvalidServiceSync.class, "local");
    }

    @Test(expected = IllegalStateException.class)
    public void testGetNoRuntime() {
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(mock(JoynrIntegrationBean.class));
        subject.get(MyServiceSync.class, "local");
    }

}
