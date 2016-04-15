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
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;

import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.HashSet;
import java.util.Set;

import javax.enterprise.inject.Any;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.util.AnnotationLiteral;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;

import io.joynr.exceptions.JoynrException;
import io.joynr.jeeintegration.ProviderWrapper;
import io.joynr.jeeintegration.api.ProviderQosFactory;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import joynr.types.ProviderQos;

/**
 * Unit tests for {@link ProviderWrapper}.
 *
 * @author clive.jevons commissioned by MaibornWolff
 */
@RunWith(MockitoJUnitRunner.class)
public class ProviderWrapperTest {

    public static interface TestServiceProviderInterface {
        String INTERFACE_NAME = "test";

        Promise<Deferred<String>> testServiceMethod(int paramOne, String paramTwo);

        Promise<Deferred<String>> testServiceMethodNoArgs();

        Promise<DeferredVoid> testServiceMethodVoidReturn();
    }

    public static interface TestServiceInterface {
        String INTERFACE_NAME = "test";

        String testServiceMethod(int paramOne, String paramTwo);

        String testServiceMethodNoArgs();

        void testServiceMethodVoidReturn();
    }

    public static class TestServiceImpl implements TestServiceInterface {

        @Override
        public String testServiceMethod(int paramOne, String paramTwo) {
            return String.format("Result: %d %s", paramOne, paramTwo);
        }

        @Override
        public String testServiceMethodNoArgs() {
            return "test";
        }

        @Override
        public void testServiceMethodVoidReturn() {
        }

    }

    public static class TestProviderQosFactory implements ProviderQosFactory {
        private ProviderQos providerQos;

        public TestProviderQosFactory(ProviderQos providerQos) {
            this.providerQos = providerQos;
        }

        @Override
        public ProviderQos create() {
            return providerQos;
        }

        @Override
        public boolean providesFor(Class<?> serviceInterface) {
            return TestServiceInterface.class.isAssignableFrom(serviceInterface);
        }
    }

    @Test
    public void testInvokeVoidReturnMethod() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethodVoidReturn");

        Object result = subject.invoke(proxy, method, new Object[0]);

        assertTrue(result instanceof Promise);
    }

    @Test
    public void testInvokeMethodWithParams() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethod", new Class[]{ Integer.TYPE,
                String.class });

        Object result = subject.invoke(proxy, method, new Object[]{ 1, "one" });

        assertTrue(result instanceof Promise);
        assertPromiseEquals(result, new TestServiceImpl().testServiceMethod(1, "one"));
    }

    @Test
    public void testInvokeMethodNoArgs() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethodNoArgs");

        Object result = subject.invoke(proxy, method, new Object[0]);

        assertTrue(result instanceof Promise);
        assertPromiseEquals(result, "test");
    }

    @SuppressWarnings("rawtypes")
    private void assertPromiseEquals(Object result, Object value) {
        assertTrue(((Promise) result).isFulfilled());
        Boolean[] ensureFulfilled = new Boolean[]{ false };
        ((Promise) result).then(new PromiseListener() {

            @Override
            public void onRejection(JoynrException error) {
                Assert.fail();
            }

            @Override
            public void onFulfillment(Object... values) {
                assertEquals(value, values[0]);
                ensureFulfilled[0] = true;
            }
        });
        assertTrue(ensureFulfilled[0]);
    }

    private ProviderWrapper createSubject() {
        return createSubject(Mockito.mock(BeanManager.class));
    }

    private ProviderWrapper createSubject(BeanManager beanManager) {
        Bean<?> bean = Mockito.mock(Bean.class);
        Mockito.doReturn(TestServiceImpl.class).when(bean).getBeanClass();
        Mockito.doReturn(new TestServiceImpl()).when(bean).create(null);
        ProviderWrapper subject = new ProviderWrapper(TestServiceInterface.class, bean, beanManager);
        return subject;
    }

    private JoynrProvider createProxy(ProviderWrapper forWrapper) {
        return (JoynrProvider) Proxy.newProxyInstance(ProviderWrapper.class.getClassLoader(), new Class[]{
                JoynrProvider.class, TestServiceInterface.class }, forWrapper);
    }

}
