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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;
import java.util.Set;

import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.Guice;

import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.JoynrRuntimeFactory;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ProviderRegistrationSettingsFactory;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.runtime.JoynrRuntime;
import joynr.exceptions.ApplicationException;
import joynr.jeeintegration.servicelocator.MyServiceProvider;
import joynr.jeeintegration.servicelocator.MyServiceSync;
import joynr.types.ProviderQos;

/**
 * Unit tests for the {@link JoynrIntegrationBean}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JoynrIntegrationBeanTest {

    private static final String LOCAL_DOMAIN = "local.domain";

    private static final String MY_CUSTOM_DOMAIN = "my.custom.domain";

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    private static class MyServiceBean implements MyServiceSync {
        @Override
        public String callMe(String parameterOne) {
            return null;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(null);
        }
    }

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    @ProviderDomain(MY_CUSTOM_DOMAIN)
    private static class CustomDomainMyServiceBean implements MyServiceSync {
        @Override
        public String callMe(String parameterOne) {
            return null;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(null);
        }
    }

    private static class MyProviderSettingsFactory implements ProviderRegistrationSettingsFactory {
        @Override
        public ProviderQos createProviderQos() {
            ProviderQos providerQos = new ProviderQos();
            providerQos.setPriority(100L);
            return providerQos;
        }

        @Override
        public String[] createGbids() {
            return new String[]{ "gbid1", "gbid2" };
        }

        @Override
        public boolean providesFor(Class<?> serviceInterface) {
            return MyServiceSync.class.isAssignableFrom(serviceInterface);
        }
    }

    @Mock
    private BeanManager beanManager;

    @Mock
    private JoynrRuntimeFactory joynrRuntimeFactory;

    @Mock
    private JoynrRuntime joynrRuntime;

    @Mock
    private ServiceProviderDiscovery serviceProviderDiscovery;

    @Mock
    private CallbackHandlerDiscovery callbackHandlerDiscovery;

    private JoynrIntegrationBean subject;

    @Before
    public void setup() {
        when(joynrRuntimeFactory.getInjector()).thenReturn(Guice.createInjector());
        when(joynrRuntimeFactory.create(any())).thenReturn(joynrRuntime);
        when(joynrRuntimeFactory.getLocalDomain()).thenReturn(LOCAL_DOMAIN);

        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                return MyServiceProvider.class;
            }
        }).when(serviceProviderDiscovery).getProviderInterfaceFor(eq(MyServiceSync.class));

        subject = new JoynrIntegrationBean(beanManager,
                                           joynrRuntimeFactory,
                                           serviceProviderDiscovery,
                                           callbackHandlerDiscovery);
    }

    @Test
    public void testInitialise() {
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(new HashSet<>());

        subject.initialise();

        verify(joynrRuntimeFactory).create(new HashSet<>());
        verify(serviceProviderDiscovery).findServiceProviderBeans();
    }

    @Test
    public void testRegisterProviderWithLocalDomain() {
        Set<Bean<?>> serviceProviderBeans = new HashSet<>();
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(MyServiceBean.class);
        serviceProviderBeans.add(bean);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        subject.initialise();

        verify(joynrRuntime).registerProvider(eq(LOCAL_DOMAIN), any(), any(), any(), eq(false), any());
    }

    @Test
    public void testRegisterProviderWithDifferentDomain() {
        Set<Bean<?>> serviceProviderBeans = new HashSet<>();
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(CustomDomainMyServiceBean.class);
        serviceProviderBeans.add(bean);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        subject.initialise();

        verify(joynrRuntime).registerProvider(eq(MY_CUSTOM_DOMAIN), any(), any(), any(), eq(false), any());
    }

    @Test
    public void testRegisterProviderUsingSettingsFromFactory() {
        // given we have a bean implementing a joynr provider...
        Set<Bean<?>> serviceProviderBeans = new HashSet<>();
        Bean bean = mock(Bean.class);
        when(bean.getBeanClass()).thenReturn(MyServiceBean.class);
        serviceProviderBeans.add(bean);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(serviceProviderBeans);

        // ...and a factory for customized provider registration settings
        MyProviderSettingsFactory settingsFactory = new MyProviderSettingsFactory();

        //... and the bean manager mock returns this factory wrapped in a bean
        doAnswer(new Answer<Object>() {
            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                Bean factoryAsBean = mock(Bean.class);
                when(factoryAsBean.getBeanClass()).thenReturn(MyProviderSettingsFactory.class);
                when(factoryAsBean.create(null)).thenReturn(settingsFactory);

                Set<Bean<?>> providerSettingsFactoryBeans = new HashSet<>();
                providerSettingsFactoryBeans.add(factoryAsBean);
                return providerSettingsFactoryBeans;
            }
        }).when(beanManager).getBeans(eq(ProviderRegistrationSettingsFactory.class), any());

        // when we initialize the subject (and register providers)
        subject.initialise();

        // then the runtime is called with the correct parameters from the factory
        ProviderQos expectedProviderQos = new ProviderQos();
        expectedProviderQos.setPriority(100L);
        String[] expcectedGbids = new String[]{ "gbid1", "gbid2" };
        verify(joynrRuntime).registerProvider(eq(LOCAL_DOMAIN),
                                              any(),
                                              eq(expectedProviderQos),
                                              eq(expcectedGbids),
                                              eq(false),
                                              any());
    }
}
