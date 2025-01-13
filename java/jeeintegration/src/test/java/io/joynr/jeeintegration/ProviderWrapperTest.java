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

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import java.io.Serializable;
import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;

import jakarta.ejb.EJBException;
import jakarta.enterprise.context.spi.CreationalContext;
import jakarta.enterprise.inject.spi.Annotated;
import jakarta.enterprise.inject.spi.Bean;
import jakarta.enterprise.inject.spi.BeanManager;
import jakarta.enterprise.inject.spi.InjectionPoint;
import jakarta.enterprise.util.AnnotationLiteral;
import com.google.inject.Inject;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnitRunner;

import com.google.inject.Injector;

import io.joynr.dispatcher.rpc.MultiReturnValuesContainer;
import io.joynr.dispatcher.rpc.annotation.JoynrMulticast;
import io.joynr.exceptions.JoynrException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.context.JoynrJeeMessageContext;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherProducer;
import io.joynr.messaging.JoynrMessageCreator;
import io.joynr.messaging.JoynrMessageMetaInfo;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.JoynrProvider;
import io.joynr.provider.Promise;
import io.joynr.provider.PromiseListener;
import io.joynr.provider.SubscriptionPublisher;
import io.joynr.provider.SubscriptionPublisherInjection;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.jeeintegration.servicelocator.MyService.CallMeWithExceptionErrorEnum;

/**
 * Unit tests for {@link ProviderWrapper}.
 */
@RunWith(MockitoJUnitRunner.class)
public class ProviderWrapperTest {

    private static final String USERNAME = "messageCreatorId";
    private static final Map<String, Serializable> expectedMessageContext = new HashMap<>();

    private static final AnnotationLiteral<io.joynr.jeeintegration.api.SubscriptionPublisher> SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL = new AnnotationLiteral<io.joynr.jeeintegration.api.SubscriptionPublisher>() {
        private static final long serialVersionUID = 1L;
    };

    public static interface TestServiceProviderInterface extends SubscriptionPublisherInjection<SubscriptionPublisher> {
        String INTERFACE_NAME = "test";

        Promise<Deferred<String>> testServiceMethod(int paramOne, String paramTwo);

        Promise<Deferred<String>> testServiceMethodNoArgs();

        Promise<DeferredVoid> testServiceMethodVoidReturn();

        Promise<DeferredVoid> assertMessageContextActive();

        Promise<DeferredVoid> testThrowsProviderRuntimeException();

        Promise<DeferredVoid> testThrowsEJBExceptionWrappingRuntimeException();

        Promise<DeferredVoid> testThrowsApplicationException();

        Promise<Deferred<Object[]>> testMultiOutMethod();
    }

    public static interface TestServiceInterface {
        String INTERFACE_NAME = "test";

        String testServiceMethod(int paramOne, String paramTwo);

        String testServiceMethodNoArgs();

        void testServiceMethodVoidReturn();

        void assertMessageContextActive();

        void testThrowsProviderRuntimeException();

        void testThrowsEJBExceptionWrappingRuntimeException();

        void testThrowsApplicationException() throws ApplicationException;

        public class MultiOutResult implements MultiReturnValuesContainer {
            @Override
            public Object[] getValues() {
                return new Object[]{ "one", "two" };
            }
        }

        MultiOutResult testMultiOutMethod();
    }

    public static interface MySubscriptionPublisher extends SubscriptionPublisher {
        @JoynrMulticast(name = "myMulticast")
        void fireMyMulticast(String someValue);

        void fireSelectiveBroadcast(String someOtherValue);

        void myValueChanged(String myValue);
    }

    public static interface TestServiceSubscriptionPublisherInjection
            extends SubscriptionPublisherInjection<MySubscriptionPublisher> {
    }

    @ServiceProvider(serviceInterface = TestServiceInterface.class)
    public static class TestServiceImpl implements TestServiceInterface {

        @Inject
        @io.joynr.jeeintegration.api.SubscriptionPublisher
        private MySubscriptionPublisher subscriptionPublisher;

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

        @Override
        public void assertMessageContextActive() {
            assertTrue(JoynrJeeMessageContext.getInstance().isActive());
        }

        @Override
        public void testThrowsProviderRuntimeException() {
            throw new ProviderRuntimeException("test");
        }

        @Override
        public void testThrowsEJBExceptionWrappingRuntimeException() {
            throw new EJBException(new RuntimeException("test"));
        }

        @Override
        public void testThrowsApplicationException() throws ApplicationException {
            throw new ApplicationException(CallMeWithExceptionErrorEnum.MY_ERROR);
        }

        @Override
        public MultiOutResult testMultiOutMethod() {
            return new MultiOutResult();
        }

    }

    @Mock
    private JoynrCallingPrincipal joynrCallingPincipal;

    @Mock
    private JoynrJeeMessageMetaInfo joynrJeeMessageContext;

    @Mock
    private SubscriptionPublisherProducer subscriptionPublisherProducer;

    @Test
    public void testInvokeVoidReturnMethod() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethodVoidReturn");

        Object result = subject.invoke(proxy, method, new Object[0]);

        assertTrue(result instanceof Promise);
    }

    private void testProxyInvokesProviderMethod(String methodName,
                                                PromiseListener promiseListener,
                                                Boolean shouldBeFulfilled) throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod(methodName);

        Object result = subject.invoke(proxy, method, new Object[0]);
        assertNotNull(result);
        assertTrue(result instanceof Promise);
        Promise<?> promise = (Promise<?>) result;
        if (shouldBeFulfilled) {
            assertTrue(promise.isFulfilled());
        } else {
            assertTrue(promise.isRejected());
        }
        promise.then(promiseListener);
    }

    @Test
    public void testInvokeMethodThrowingProviderRuntimeException() throws Throwable {
        testProxyInvokesProviderMethod("testThrowsProviderRuntimeException", new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                fail("Should never get here");
            }

            @Override
            public void onRejection(JoynrException error) {
                assertTrue(error instanceof ProviderRuntimeException);
            }
        }, false);
    }

    @Test
    public void testInvokeMethodThrowingEJBExceptionWrappingRuntimeException() throws Throwable {
        testProxyInvokesProviderMethod("testThrowsEJBExceptionWrappingRuntimeException", new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                fail("Should never get here");
            }

            @Override
            public void onRejection(JoynrException error) {
                assertTrue(error instanceof ProviderRuntimeException);
            }
        }, false);
    }

    @Test
    public void testInvokeMethodThrowingApplicationException() throws Throwable {
        testProxyInvokesProviderMethod("testThrowsApplicationException", new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                fail("Should never get here");
            }

            @Override
            public void onRejection(JoynrException error) {
                assertTrue(error instanceof ApplicationException);
                String expectedMessage = "ErrorValue: " + CallMeWithExceptionErrorEnum.MY_ERROR;
                assertEquals(((ApplicationException) error).getMessage(), expectedMessage);
            }
        }, false);
    }

    @Test
    public void testInvokeMultiOutMethod() throws Throwable {
        testProxyInvokesProviderMethod("testMultiOutMethod", new PromiseListener() {
            @Override
            public void onFulfillment(Object... values) {
                assertArrayEquals(new Object[]{ "one", "two" }, values);
            }

            @Override
            public void onRejection(JoynrException error) {
                fail("Shouldn't be here.");
            }
        }, true);
    }

    @Test
    public void testInvokeMethodWithParams() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethod",
                                                                     new Class[]{ Integer.TYPE, String.class });

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

    @Test
    public void testSetSubscriptionPublisherDoesNotActivateScope() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceSubscriptionPublisherInjection.class.getMethod("setSubscriptionPublisher",
                                                                                  new Class[]{
                                                                                          SubscriptionPublisher.class });

        subject.invoke(proxy, method, new Object[]{ mock(MySubscriptionPublisher.class) });
        assertFalse(JoynrJeeMessageContext.getInstance().isActive());
    }

    @Test
    public void testSetSubscriptionPublisherRegistersWithProducer() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceSubscriptionPublisherInjection.class.getMethod("setSubscriptionPublisher",
                                                                                  new Class[]{
                                                                                          SubscriptionPublisher.class });

        subject.invoke(proxy, method, new Object[]{ mock(MySubscriptionPublisher.class) });
        verify(subscriptionPublisherProducer).add(any(), eq(TestServiceImpl.class));
    }

    @Test
    public void testMessageScopeActivated() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("assertMessageContextActive");

        assertFalse(JoynrJeeMessageContext.getInstance().isActive());
        subject.invoke(proxy, method, new Object[0]);
        assertFalse(JoynrJeeMessageContext.getInstance().isActive());
    }

    @Test
    public void testPrincipalCopied() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethodNoArgs");

        subject.invoke(proxy, method, new Object[0]);

        verify(joynrCallingPincipal).setUsername(USERNAME);
    }

    @Test
    public void testMessageContextCopied() throws Throwable {
        ProviderWrapper subject = createSubject();
        JoynrProvider proxy = createProxy(subject);

        Method method = TestServiceProviderInterface.class.getMethod("testServiceMethodNoArgs");

        subject.invoke(proxy, method, new Object[0]);

        verify(joynrJeeMessageContext).setMessageContext(expectedMessageContext);
    }

    private void assertPromiseEquals(Object result, Object value) {
        assertTrue(((Promise<?>) result).isFulfilled());
        Boolean[] ensureFulfilled = new Boolean[]{ false };
        ((Promise<?>) result).then(new PromiseListener() {

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

    @SuppressWarnings("rawtypes")
    private ProviderWrapper createSubject(BeanManager beanManager) {
        Injector injector = mock(Injector.class);
        JoynrMessageCreator joynrMessageCreator = mock(JoynrMessageCreator.class);
        when(injector.getInstance(eq(JoynrMessageCreator.class))).thenReturn(joynrMessageCreator);
        when(joynrMessageCreator.getMessageCreatorId()).thenReturn(USERNAME);
        Bean<?> joynrCallingPrincipalBean = mock(Bean.class);
        when(beanManager.getBeans(JoynrCallingPrincipal.class)).thenReturn(new HashSet<Bean<?>>(Arrays.asList(joynrCallingPrincipalBean)));
        when(beanManager.getReference(eq(joynrCallingPrincipalBean),
                                      eq(JoynrCallingPrincipal.class),
                                      Mockito.<CreationalContext> any())).thenReturn(joynrCallingPincipal);
        Bean<?> bean = mock(Bean.class);
        doReturn(TestServiceImpl.class).when(bean).getBeanClass();
        doReturn(new TestServiceImpl()).when(beanManager).getReference(bean, TestServiceInterface.class, null);

        JoynrMessageMetaInfo joynrMessageContext = mock(JoynrMessageMetaInfo.class);
        when(injector.getInstance(eq(JoynrMessageMetaInfo.class))).thenReturn(joynrMessageContext);
        when(joynrMessageContext.getMessageContext()).thenReturn(expectedMessageContext);
        Bean<?> joynrMessageContextBean = mock(Bean.class);
        when(beanManager.getBeans(JoynrJeeMessageMetaInfo.class)).thenReturn(new HashSet<Bean<?>>(Arrays.asList(joynrMessageContextBean)));
        when(beanManager.getReference(eq(joynrMessageContextBean),
                                      eq(JoynrJeeMessageMetaInfo.class),
                                      Mockito.any())).thenReturn(joynrJeeMessageContext);

        // Setup mock SubscriptionPublisherProducer instance in mock bean manager
        Bean<?> subscriptionPublisherProducerBean = mock(Bean.class);
        doReturn(new HashSet<Bean<?>>(Arrays.asList(subscriptionPublisherProducerBean))).when(beanManager)
                                                                                        .getBeans(eq(SubscriptionPublisherProducer.class));
        when(beanManager.getReference(eq(subscriptionPublisherProducerBean),
                                      eq(SubscriptionPublisherProducer.class),
                                      any())).thenReturn(subscriptionPublisherProducer);

        // Setup mock meta data so that subscription publisher can be injected
        InjectionPoint subscriptionPublisherInjectionPoint = mock(InjectionPoint.class);
        when(bean.getInjectionPoints()).thenReturn(new HashSet<InjectionPoint>(Arrays.asList(subscriptionPublisherInjectionPoint)));
        when(subscriptionPublisherInjectionPoint.getQualifiers()).thenReturn(new HashSet<Annotation>(Arrays.asList((Annotation) SUBSCRIPTION_PUBLISHER_ANNOTATION_LITERAL)));
        Annotated annotated = mock(Annotated.class);
        when(subscriptionPublisherInjectionPoint.getAnnotated()).thenReturn(annotated);
        when(annotated.getBaseType()).thenReturn(MySubscriptionPublisher.class);

        ProviderWrapper subject = new ProviderWrapper(bean, beanManager, injector);
        return subject;
    }

    private JoynrProvider createProxy(ProviderWrapper forWrapper) {
        return (JoynrProvider) Proxy.newProxyInstance(ProviderWrapper.class.getClassLoader(),
                                                      new Class[]{ JoynrProvider.class, TestServiceInterface.class },
                                                      forWrapper);
    }

}
