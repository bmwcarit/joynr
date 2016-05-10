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

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.JoynrRuntimeFactory;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.runtime.JoynrRuntime;

import java.util.HashSet;

import javax.enterprise.inject.spi.BeanManager;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.Guice;

/**
 * Unit tests for the {@link JoynrIntegrationBean}.
 */
@RunWith(MockitoJUnitRunner.class)
public class JoynrIntegrationBeanTest {

    @Test
    public void testInitialise() {
        BeanManager beanManager = mock(BeanManager.class);
        JoynrRuntimeFactory joynrRuntimeFactory = mock(JoynrRuntimeFactory.class);
        when(joynrRuntimeFactory.getInjector()).thenReturn(Guice.createInjector());
        JoynrRuntime joynrRuntime = mock(JoynrRuntime.class);
        when(joynrRuntimeFactory.create(new HashSet<>())).thenReturn(joynrRuntime);
        ServiceProviderDiscovery serviceProviderDiscovery = mock(ServiceProviderDiscovery.class);
        when(serviceProviderDiscovery.findServiceProviderBeans()).thenReturn(new HashSet<>());

        JoynrIntegrationBean subject = new JoynrIntegrationBean(beanManager,
                                                                joynrRuntimeFactory,
                                                                serviceProviderDiscovery);
        subject.initialise();

        verify(joynrRuntimeFactory).create(new HashSet<>());
        verify(serviceProviderDiscovery).findServiceProviderBeans();
    }
}
