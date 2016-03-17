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
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;

import java.io.Serializable;
import java.util.HashSet;
import java.util.Set;

import javax.ejb.Stateless;
import javax.enterprise.inject.Any;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.util.AnnotationLiteral;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.provider.JoynrProvider;

/**
 * Unit tests for {@link ServiceProviderDiscovery}.
 *
 * @author clive.jevons commissioned by MaibornWolff
 */
@RunWith(MockitoJUnitRunner.class)
public class ServiceProviderDiscoveryTest {

    private interface DummyInterfaceProvider extends JoynrProvider {
    }

    private interface DummyInterfaceBCI {
    }

    @ServiceProvider(serviceInterface = DummyInterfaceBCI.class)
    @Stateless
    private class DummyBeanOne implements DummyInterfaceBCI {
    }

    @Stateless
    private class DummyBeanTwo {
    }

    @SuppressWarnings({ "unchecked", "serial" })
    @Test
    public void testFindServiceProviderBeans() {
        BeanManager mockBeanManager = mock(BeanManager.class);

        Bean<DummyBeanOne> mockBeanOne = mock(Bean.class);
        Mockito.doReturn(DummyBeanOne.class).when(mockBeanOne).getBeanClass();
        Bean<DummyBeanOne> mockBeanTwo = mock(Bean.class);
        Mockito.doReturn(DummyBeanTwo.class).when(mockBeanTwo).getBeanClass();

        Set<Bean<?>> beans = new HashSet<>();
        beans.add(mockBeanOne);
        beans.add(mockBeanTwo);
        Mockito.when(mockBeanManager.getBeans(Object.class, new AnnotationLiteral<Any>() {
        })).thenReturn(beans);

        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mockBeanManager);

        Set<Bean<?>> result = subject.findServiceProviderBeans();

        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.iterator().next().getBeanClass().equals(DummyBeanOne.class));
    }

    @Test
    public void testFindProviderForBCI() {
        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mock(BeanManager.class));
        Class<?> result = subject.getProviderInterfaceFor(DummyInterfaceBCI.class);
        assertNotNull(result);
        assertEquals(DummyInterfaceProvider.class, result);
    }

    @Test
    public void testFindProviderForNonBCI() {
        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mock(BeanManager.class));
        Class<?> result = subject.getProviderInterfaceFor(Serializable.class);
        assertNotNull(result);
        assertEquals(Serializable.class, result);
    }

}
