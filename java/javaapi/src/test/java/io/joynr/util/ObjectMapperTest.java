/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

import static org.mockito.Matchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Matchers;
import org.mockito.Mock;
import org.mockito.runners.MockitoJUnitRunner;

@RunWith(MockitoJUnitRunner.class)
public class ObjectMapperTest {
    private class A {
    };

    private class B {
    };

    private class C {
    };

    @Mock
    com.fasterxml.jackson.databind.ObjectMapper realObjectMapper;

    @Test
    public void registerSubtypes_NewEntries() {
        ObjectMapper objectMapper = new ObjectMapper(realObjectMapper);
        objectMapper.registerSubtypes(A.class, B.class);
        verify(realObjectMapper).registerSubtypes(eq(A.class), eq(B.class));
        objectMapper.registerSubtypes(C.class);
        verify(realObjectMapper).registerSubtypes(eq(C.class));
        verify(realObjectMapper, times(2)).registerSubtypes(Matchers.<Class<?>> anyVararg());
    }

    @Test
    public void registerSubtypes_NewAndExisitingEntries() {
        ObjectMapper objectMapper = new ObjectMapper(realObjectMapper);
        objectMapper.registerSubtypes(A.class, B.class);
        verify(realObjectMapper).registerSubtypes(eq(A.class), eq(B.class));
        objectMapper.registerSubtypes(A.class, C.class);
        verify(realObjectMapper).registerSubtypes(eq(C.class));
        verify(realObjectMapper, times(2)).registerSubtypes(Matchers.<Class<?>> anyVararg());
    }

    @Test
    public void registerSubtypes_OmitCallForExisitingEntries() {
        ObjectMapper objectMapper = new ObjectMapper(realObjectMapper);
        objectMapper.registerSubtypes(A.class, B.class);
        objectMapper.registerSubtypes(A.class, B.class);
        verify(realObjectMapper).registerSubtypes(eq(A.class), eq(B.class));
        verify(realObjectMapper, times(1)).registerSubtypes(Matchers.<Class<?>> anyVararg());
    }

}
