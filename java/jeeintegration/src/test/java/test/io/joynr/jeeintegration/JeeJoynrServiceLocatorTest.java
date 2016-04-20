/**
 *
 */
package test.io.joynr.jeeintegration;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;
import test.io.joynr.jeeintegration.servicelocator.MyInvalidServiceBCI;
import test.io.joynr.jeeintegration.servicelocator.MyServiceBCI;
import test.io.joynr.jeeintegration.servicelocator.MyServiceProxy;
import test.io.joynr.jeeintegration.servicelocator.MyUnproxiedServiceBCI;

/**
 * Unit tests for {@link JeeJoynrServiceLocator}.
 *
 * @author clive.jevons commissioned by MaibornWolff
 */
@RunWith(MockitoJUnitRunner.class)
public class JeeJoynrServiceLocatorTest {

    @Test
    public void testGet() {
        MyServiceProxy myJoynrProxy = mock(MyServiceProxy.class);
        when(myJoynrProxy.callMe("one")).thenReturn("two");
        @SuppressWarnings("unchecked")
        ProxyBuilder<MyServiceProxy> proxyBuilder = mock(ProxyBuilder.class);
        when(proxyBuilder.setMessagingQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.build()).thenReturn(myJoynrProxy);
        JoynrRuntime joynrRuntime = mock(JoynrRuntime.class);
        when(joynrRuntime.getProxyBuilder("local", MyServiceProxy.class)).thenReturn(proxyBuilder);
        JoynrIntegrationBean joynrIntegrationBean = mock(JoynrIntegrationBean.class);
        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(joynrIntegrationBean);

        MyServiceBCI result = subject.get(MyServiceBCI.class, "local");

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertNotNull(callResult);
        assertEquals("two", callResult);
        Mockito.verify(myJoynrProxy).callMe("one");
    }

    @Test
    public void testGetWithTtl() {
        MyServiceProxy myJoynrProxy = mock(MyServiceProxy.class);
        when(myJoynrProxy.callMe("one")).thenReturn("two");
        @SuppressWarnings("unchecked")
        ProxyBuilder<MyServiceProxy> proxyBuilder = mock(ProxyBuilder.class);
        when(proxyBuilder.setMessagingQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.build()).thenReturn(myJoynrProxy);
        JoynrRuntime joynrRuntime = mock(JoynrRuntime.class);
        when(joynrRuntime.getProxyBuilder("local", MyServiceProxy.class)).thenReturn(proxyBuilder);
        JoynrIntegrationBean joynrIntegrationBean = mock(JoynrIntegrationBean.class);
        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(joynrIntegrationBean);

        MyServiceBCI result = subject.get(MyServiceBCI.class, "local", 10000L);

        assertNotNull(result);

        String callResult = result.callMe("one");

        assertNotNull(callResult);
        assertEquals("two", callResult);
        Mockito.verify(myJoynrProxy).callMe("one");
        ArgumentCaptor<MessagingQos> messagingQosCaptor = ArgumentCaptor.forClass(MessagingQos.class);
        Mockito.verify(proxyBuilder).setMessagingQos(messagingQosCaptor.capture());
        MessagingQos messagingQosParam = messagingQosCaptor.getValue();
        assertEquals(10000L, messagingQosParam.getRoundTripTtl_ms());
    }

    @Test
    public void testGetWithMessagingAndDiscoveryQos() {
        MyServiceProxy myJoynrProxy = mock(MyServiceProxy.class);
        when(myJoynrProxy.callMe("one")).thenReturn("two");
        @SuppressWarnings("unchecked")
        ProxyBuilder<MyServiceProxy> proxyBuilder = mock(ProxyBuilder.class);
        when(proxyBuilder.setMessagingQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.setDiscoveryQos(Mockito.any())).thenReturn(proxyBuilder);
        when(proxyBuilder.build()).thenReturn(myJoynrProxy);
        JoynrRuntime joynrRuntime = mock(JoynrRuntime.class);
        when(joynrRuntime.getProxyBuilder("local", MyServiceProxy.class)).thenReturn(proxyBuilder);
        JoynrIntegrationBean joynrIntegrationBean = mock(JoynrIntegrationBean.class);
        when(joynrIntegrationBean.getRuntime()).thenReturn(joynrRuntime);
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(joynrIntegrationBean);

        MessagingQos messagingQos = new MessagingQos();
        DiscoveryQos discoveryQos = new DiscoveryQos();
        MyServiceBCI result = subject.get(MyServiceBCI.class, "local", messagingQos, discoveryQos);

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

        subject.get(MyUnproxiedServiceBCI.class, "local");
    }

    @Test(expected = IllegalStateException.class)
    public void testGetNoRuntime() {
        JeeJoynrServiceLocator subject = new JeeJoynrServiceLocator(mock(JoynrIntegrationBean.class));
        subject.get(MyServiceBCI.class, "local");
    }

}
