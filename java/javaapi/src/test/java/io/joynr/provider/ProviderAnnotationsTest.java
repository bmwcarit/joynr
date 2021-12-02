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
package io.joynr.provider;

import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.junit.MockitoJUnitRunner;

import joynr.tests.DefaulttestProvider;
import joynr.tests.testProvider;

@RunWith(MockitoJUnitRunner.class)
public class ProviderAnnotationsTest {

    @Test(expected = IllegalArgumentException.class)
    public void getExceptionForPrimitiveProviderInstanceWithouthAnnotation() {
        ProviderAnnotations.getInterfaceName("");
    }

    @Test(expected = IllegalArgumentException.class)
    public void getExceptionForProviderInstanceWithouthAnnotation() {
        ProviderAnnotations.getInterfaceName(new ProviderAnnotations());
    }

    @Test
    public void getAnnotationInProviderObject() {
        assertThat(ProviderAnnotations.getInterfaceName(testProvider.class), equalTo("tests/test"));
        assertThat(ProviderAnnotations.getInterfaceName(DefaulttestProvider.class), equalTo("tests/test"));
        assertThat(ProviderAnnotations.getInterfaceName(new DefaulttestProvider()), equalTo("tests/test"));
    }
}
