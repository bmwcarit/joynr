package io.joynr.generator.templates.util;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FField;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FTypeRef;
import org.franca.core.franca.FrancaFactory;
import org.junit.Test;

import com.google.inject.Guice;

public class TypeUtilTest {

    @Test
    public void testRecurstiveStruct() throws Exception {
        FStructType structType = FrancaFactory.eINSTANCE.createFStructType();
        structType.setName("TestStruct");
        FField field = FrancaFactory.eINSTANCE.createFField();
        field.setName("exampleField");
        field.setArray(true);
        FTypeRef typeRef = FrancaFactory.eINSTANCE.createFTypeRef();
        typeRef.setDerived(structType);
        field.setType(typeRef);
        structType.getElements().add(field);
        TypeUtil typeUtil = Guice.createInjector().getInstance(TypeUtil.class);
        FCompoundType result = typeUtil.getCompoundType(structType);
        assertEquals(structType, result);
    }

}
