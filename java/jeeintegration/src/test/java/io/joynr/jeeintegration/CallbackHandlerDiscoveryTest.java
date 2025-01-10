/*-
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
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.util.HashSet;
import java.util.Set;

import jakarta.ejb.Stateless;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.jeeintegration.api.CallbackHandler;
import io.joynr.jeeintegration.servicelocator.MyServiceCallbackHandler;
import io.joynr.proxy.StatelessAsyncCallback;
import joynr.jeeintegration.servicelocator.MyServiceStatelessAsyncCallback;

@RunWith(MockitoJUnitRunner.class)
public class CallbackHandlerDiscoveryTest {

    @Mock
    private BeanManager beanManager;

    @InjectMocks
    private CallbackHandlerDiscovery subject;

    private interface MyOtherService {
        void test();
    }

    @Stateless
    private class MyOtherBean implements MyOtherService {
        @Override
        public void test() {
        }
    }

    @Test
    public void testDiscoversCallbackHandler() {
        Bean callbackHandlerBean = mock(Bean.class);
        when(callbackHandlerBean.getBeanClass()).thenReturn(MyServiceCallbackHandler.class);
        Bean nonCallbackHandlerBean = mock(Bean.class);
        when(nonCallbackHandlerBean.getBeanClass()).thenReturn(MyOtherBean.class);
        Set<Bean<?>> beans = new HashSet<>();
        beans.add(callbackHandlerBean);
        beans.add(nonCallbackHandlerBean);
        when(beanManager.getBeans(eq(Object.class), any())).thenReturn(beans);
        when(beanManager.getReference(eq(callbackHandlerBean),
                                      eq(MyServiceStatelessAsyncCallback.class),
                                      any())).thenReturn(new MyServiceCallbackHandler());

        Set<StatelessAsyncCallback> discovered = new HashSet<>();
        subject.forEach(discovered::add);

        assertEquals(1, discovered.size());
        assertEquals("useCase", discovered.iterator().next().getUseCase());

        verify(nonCallbackHandlerBean).getBeanClass();
    }

    // no @UsedBy
    public interface MyInvalidCallback extends StatelessAsyncCallback {
    }

    @SuppressWarnings("CdiManagedBeanInconsistencyInspection")
    @Stateless
    @CallbackHandler
    public class MyInvalidCallbackHandler implements MyInvalidCallback {
        @Override
        public String getUseCase() {
            return null;
        }
    }

    @Test
    public void testStatelessAsyncWithoutUsedByNotDiscovered() {
        Bean callbackHandlerBean = mock(Bean.class);
        when(callbackHandlerBean.getBeanClass()).thenReturn(MyInvalidCallbackHandler.class);
        Set<Bean<?>> beans = new HashSet<>();
        beans.add(callbackHandlerBean);
        when(beanManager.getBeans(eq(Object.class), any())).thenReturn(beans);

        subject.forEach(statelessAsyncCallback -> fail("Should not be discovered"));
    }
}
