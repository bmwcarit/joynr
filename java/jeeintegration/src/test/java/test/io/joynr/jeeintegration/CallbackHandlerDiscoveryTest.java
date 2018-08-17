/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import javax.ejb.Stateless;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;

import com.google.common.collect.Sets;
import io.joynr.proxy.StatelessAsyncCallback;
import joynr.jeeintegration.servicelocator.MyServiceStatelessAsyncCallback;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InjectMocks;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import test.io.joynr.jeeintegration.servicelocator.MyServiceCallbackHandler;

import java.util.HashSet;
import java.util.Set;

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
        Set<Bean<?>> beans = Sets.newHashSet(callbackHandlerBean, nonCallbackHandlerBean);
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
}
