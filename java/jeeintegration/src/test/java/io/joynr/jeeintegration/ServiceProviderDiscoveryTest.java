/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.lenient;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import java.io.Serializable;
import java.util.HashSet;
import java.util.Set;

import jakarta.ejb.Stateless;
import jakarta.enterprise.inject.Any;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.enterprise.util.AnnotationLiteral;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.servicelocator.MyInvalidServiceSync;
import joynr.exceptions.ApplicationException;
import joynr.jeeintegration.servicelocator.MyServiceProvider;
import joynr.jeeintegration.servicelocator.MyServiceSync;

/**
 * Unit tests for {@link ServiceProviderDiscovery}.
 */
@RunWith(MockitoJUnitRunner.class)
public class ServiceProviderDiscoveryTest {

    @ServiceProvider(serviceInterface = MyServiceSync.class)
    @Stateless
    private class DummyBeanOne implements MyServiceSync {

        @Override
        public String callMe(String parameterOne) throws JoynrRuntimeException {
            return "DummyBeanOne";
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(CallMeWithExceptionErrorEnum.MY_ERROR);
        }
    }

    @Stateless
    private class DummyBeanTwo implements MyServiceSync {

        @Override
        public String callMe(String parameterOne) throws JoynrRuntimeException {
            return "DummyBeanTwo";
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(CallMeWithExceptionErrorEnum.MY_ERROR);
        }
    }

    @ServiceProvider(serviceInterface = MyInvalidServiceSync.class)
    @Stateless
    private class DummyBeanThree implements MyInvalidServiceSync {

        @Override
        public void test() {
            //do nothing
        }
    }

    @ServiceProvider(serviceInterface = MyServiceProvider.class)
    @Stateless
    private class DummyBeanFour implements MyServiceSync {

        @Override
        public String callMe(String parameterOne) {
            return parameterOne;
        }

        @Override
        public void callMeWithException() throws ApplicationException {
            throw new ApplicationException(CallMeWithExceptionErrorEnum.MY_ERROR);
        }
    }

    @SuppressWarnings({ "unchecked", "serial" })
    @Test
    public void testFindServiceProviderBeans() {
        BeanManager mockBeanManager = mock(BeanManager.class);

        Bean<DummyBeanOne> mockBeanOne = mock(Bean.class);
        doReturn(DummyBeanOne.class).when(mockBeanOne).getBeanClass();
        Bean<DummyBeanTwo> mockBeanTwo = mock(Bean.class);
        doReturn(DummyBeanTwo.class).when(mockBeanTwo).getBeanClass();

        Set<Bean<?>> beans = new HashSet<>();
        beans.add(mockBeanOne);
        beans.add(mockBeanTwo);
        when(mockBeanManager.getBeans(Object.class, new AnnotationLiteral<Any>() {
        })).thenReturn(beans);

        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mockBeanManager);

        Set<Bean<?>> result = subject.findServiceProviderBeans();

        assertNotNull(result);
        assertEquals(1, result.size());
        assertTrue(result.iterator().next().getBeanClass().equals(DummyBeanOne.class));
    }

    @Test
    public void testFindProviderForCorrectInterface() {
        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mock(BeanManager.class));
        Class<?> result = subject.getProviderInterfaceFor(MyServiceSync.class);
        assertNotNull(result);
        assertEquals(MyServiceProvider.class, result);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testFindProviderForInterfaceWithoutServiceProviderAnnotation() {
        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mock(BeanManager.class));
        subject.getProviderInterfaceFor(Serializable.class);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testFindProviderForInterfaceWithCorruptServiceProviderAnnotation() {
        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mock(BeanManager.class));
        subject.getProviderInterfaceFor(MyInvalidServiceSync.class);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testInvalidServiceInterfaceSpecified() {
        BeanManager mockBeanManager = mock(BeanManager.class);

        Bean<DummyBeanOne> mockBeanOne = mock(Bean.class);
        // By chance depending on the implemenation of the Set<Bean<?>> below
        // this might called and hence lenient() must be used.
        lenient().doReturn(DummyBeanOne.class).when(mockBeanOne).getBeanClass();
        Bean<DummyBeanFour> mockBeanThree = mock(Bean.class);
        doReturn(DummyBeanThree.class).when(mockBeanThree).getBeanClass();

        Set<Bean<?>> beans = new HashSet<>();
        beans.add(mockBeanOne);
        beans.add(mockBeanThree);
        when(mockBeanManager.getBeans(Object.class, new AnnotationLiteral<Any>() {
        })).thenReturn(beans);

        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mockBeanManager);

        subject.findServiceProviderBeans();
        fail("Shouldn't be able to get here with an invalid bean (DummyBeanThree)");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testWrongServiceInterfaceSpecified() {
        BeanManager mockBeanManager = mock(BeanManager.class);

        Bean<DummyBeanOne> mockBeanOne = mock(Bean.class);
        // By chance depending on the implemenation of the Set<Bean<?>> below
        // this might called and hence lenient() must be used.
        lenient().doReturn(DummyBeanOne.class).when(mockBeanOne).getBeanClass();
        Bean<DummyBeanFour> mockBeanFour = mock(Bean.class);
        doReturn(DummyBeanFour.class).when(mockBeanFour).getBeanClass();

        Set<Bean<?>> beans = new HashSet<>();
        beans.add(mockBeanOne);
        beans.add(mockBeanFour);
        when(mockBeanManager.getBeans(Object.class, new AnnotationLiteral<Any>() {
        })).thenReturn(beans);

        ServiceProviderDiscovery subject = new ServiceProviderDiscovery(mockBeanManager);

        subject.findServiceProviderBeans();
        fail("Shouldn't be able to get here with an invalid bean (DummyBeanFour)");
    }
}
