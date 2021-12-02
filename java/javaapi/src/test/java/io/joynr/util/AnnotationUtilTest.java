/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
package io.joynr.util;

import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.instanceOf;
import static org.hamcrest.collection.IsCollectionWithSize.hasSize;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;

import java.lang.annotation.Annotation;
import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.util.Collection;
import java.util.stream.Collectors;

import javax.inject.Qualifier;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import io.joynr.JoynrVersion;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.provider.JoynrInterface;
import io.joynr.provider.ProviderAnnotations;
import io.joynr.proxy.ReplyContext;
import joynr.tests.DefaulttestProvider;
import joynr.tests.test;
import joynr.tests.testProvider;
import joynr.tests.testStatelessAsyncCallback;
import joynr.tests.testTypes.TestEnum;

@RunWith(MockitoJUnitRunner.class)
public class AnnotationUtilTest {

    @Test
    public void testgetAnnotationsRecursive() {
        Collection<Annotation> annotations = AnnotationUtil.getAnnotationsRecursive(DefaulttestProvider.class);

        Collection<? extends Annotation> joynrInterfaceAnnotations = annotations.stream()
                                                                                .filter(JoynrInterface.class::isInstance)
                                                                                .collect(Collectors.toList());
        assertThat(joynrInterfaceAnnotations, hasSize(1));

        Annotation interfaceNameAnnotation = joynrInterfaceAnnotations.iterator().next();
        assertThat(interfaceNameAnnotation, instanceOf(JoynrInterface.class));
        assertThat(((JoynrInterface) interfaceNameAnnotation).name(), equalTo("tests/test"));
    }

    @Test
    public void testInterfaceNameAnnotation() {
        JoynrInterface joynrInterfaceAnnotations = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                                JoynrInterface.class);
        assertThat(joynrInterfaceAnnotations.name(), equalTo("tests/test"));
    }

    @Test
    public void testInterfaceClassAnnotation() {
        JoynrInterface interfaceClassAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                               JoynrInterface.class);
        assertEquals(interfaceClassAnnotation.provider(), testProvider.class);
        assertEquals(interfaceClassAnnotation.provides(), test.class);
    }

    @Test
    public void testMajorVersionAnnotation() {
        JoynrVersion joynrVersionnAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                            JoynrVersion.class);
        assertThat(joynrVersionnAnnotation.major(), equalTo(47));
    }

    @Test
    public void testMinorVersionAnnotation() {
        JoynrVersion joynrVersionnAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                            JoynrVersion.class);
        assertThat(joynrVersionnAnnotation.minor(), equalTo(11));

    }

    @Test(expected = IllegalArgumentException.class)
    public void getExceptionForProviderInstanceWithouthAnnotation() {
        ProviderAnnotations.getInterfaceName("");
    }

    @Test
    public void getAnnotationInProxyObject() {
        Object fixture = Proxy.newProxyInstance(this.getClass().getClassLoader(),
                                                new Class<?>[]{ testProvider.class },
                                                new InvocationHandler() {

                                                    @Override
                                                    public Object invoke(Object proxy,
                                                                         Method method,
                                                                         Object[] args) throws Throwable {
                                                        return null;
                                                    }
                                                });
        assertThat(ProviderAnnotations.getInterfaceName(fixture), equalTo("tests/test"));
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target(ElementType.METHOD)
    @interface DirectAnnotation {
    }

    class MyTestStatelessAsyncCallback implements testStatelessAsyncCallback {
        @Override
        @DirectAnnotation
        public void getEnumAttributeSuccess(TestEnum enumAttribute, ReplyContext replyContext) {
            // noop
        }

        @Override
        public String getUseCase() {
            return "attribute test";
        }
    }

    @Test
    public void testGetAnnotationFromMethodRecursively() throws Exception {
        StatelessCallbackCorrelation result = AnnotationUtil.getAnnotation(getMethodWithAnnotation(),
                                                                           StatelessCallbackCorrelation.class);
        assertNotNull(result);
    }

    @Test
    public void testGetDirectAnnotationForMethod() throws Exception {
        DirectAnnotation result = AnnotationUtil.getAnnotation(getMethodWithAnnotation(), DirectAnnotation.class);
        assertNotNull(result);
    }

    @Test
    public void testGetAnnotationForMethodReturnsNullForNonExistentAnnotation() throws Exception {
        Qualifier result = AnnotationUtil.getAnnotation(getMethodWithAnnotation(), Qualifier.class);
        assertNull(result);
    }

    class MyChildClass extends MyTestStatelessAsyncCallback {
        @Override
        public void getEnumAttributeSuccess(TestEnum enumAttribute, ReplyContext replyContext) {
            //noop
        }
    }

    @Test
    public void testGetAnnotationFromSuperclassMethod() throws Exception {
        Method method = MyChildClass.class.getMethod("getEnumAttributeSuccess", TestEnum.class, ReplyContext.class);
        DirectAnnotation result = AnnotationUtil.getAnnotation(method, DirectAnnotation.class);
        assertNotNull(result);
    }

    private Method getMethodWithAnnotation() throws NoSuchMethodException {
        return MyTestStatelessAsyncCallback.class.getMethod("getEnumAttributeSuccess",
                                                            TestEnum.class,
                                                            ReplyContext.class);
    }
}
