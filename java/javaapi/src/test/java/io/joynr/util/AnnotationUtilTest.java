package io.joynr.util;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import static org.hamcrest.Matchers.equalTo;
import static org.hamcrest.Matchers.instanceOf;
import static org.hamcrest.collection.IsCollectionWithSize.hasSize;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import io.joynr.provider.InterfaceClass;
import io.joynr.provider.InterfaceName;
import io.joynr.provider.MajorVersion;
import io.joynr.provider.MinorVersion;

import java.lang.annotation.Annotation;
import java.util.Collection;

import joynr.tests.DefaulttestProvider;
import joynr.tests.testProvider;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.common.base.Predicates;
import com.google.common.collect.Collections2;

@RunWith(MockitoJUnitRunner.class)
public class AnnotationUtilTest {

    @Test
    public void testgetAnnotationsRecursive() {
        Collection<Annotation> annotations = AnnotationUtil.getAnnotationsRecursive(DefaulttestProvider.class);

        Collection<? extends Annotation> interfaceNameAnnotations = Collections2.filter(annotations,
                                                                                        Predicates.instanceOf(InterfaceName.class));
        assertThat(interfaceNameAnnotations, hasSize(1));

        Annotation interfaceNameAnnotation = interfaceNameAnnotations.iterator().next();
        assertThat(interfaceNameAnnotation, instanceOf(InterfaceName.class));
        assertThat(((InterfaceName) interfaceNameAnnotation).value(), equalTo(testProvider.INTERFACE_NAME));
    }

    @Test
    public void testInterfaceNameAnnotation() {
        InterfaceName interfaceNameAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                             InterfaceName.class);
        assertThat(interfaceNameAnnotation.value(), equalTo(testProvider.INTERFACE_NAME));
    }

    @Test
    public void testInterfaceClassAnnotation() {
        InterfaceClass interfaceClassAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                               InterfaceClass.class);
        assertEquals(interfaceClassAnnotation.value(), testProvider.class);
    }

    @Test
    public void testMajorVersionAnnotation() {
        MajorVersion majorVersionAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                           MajorVersion.class);
        assertThat(majorVersionAnnotation.value(), equalTo(testProvider.MAJOR_VERSION));
    }

    @Test
    public void testMinorVersionAnnotation() {
        MinorVersion majorVersionAnnotation = AnnotationUtil.getAnnotation(DefaulttestProvider.class,
                                                                           MinorVersion.class);
        assertThat(majorVersionAnnotation.value(), equalTo(testProvider.MINOR_VERSION));
    }

}