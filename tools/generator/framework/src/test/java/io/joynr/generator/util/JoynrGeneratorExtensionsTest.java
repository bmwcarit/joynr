package io.joynr.generator.util;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;
import static org.mockito.Mockito.*;

import java.net.URL;
import java.util.ArrayList;
import java.util.Iterator;

import io.joynr.generator.loading.ModelLoader;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FBasicTypeId;
import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FField;
import org.franca.core.franca.FMethod;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FType;
import org.franca.core.franca.FTypeRef;
import org.franca.core.franca.FrancaFactory;
import org.junit.Test;
import org.mockito.internal.stubbing.answers.CallsRealMethods;
import org.mockito.invocation.InvocationOnMock;

public class JoynrGeneratorExtensionsTest {

    @Test
    public void testRecurstiveStruct() throws Exception {
        FStructType structType = FrancaFactory.eINSTANCE.createFStructType();
        structType.setName("TestStruct");
        FField field = FrancaFactory.eINSTANCE.createFField();
        field.setName("exampleField");
        field.setArray("[]");
        FTypeRef typeRef = FrancaFactory.eINSTANCE.createFTypeRef();
        typeRef.setDerived(structType);
        field.setType(typeRef);
        structType.getElements().add(field);
        JoynrGeneratorExtensions extension = mock(JoynrGeneratorExtensions.class, new CallsRealMethods());
        FCompoundType result = extension.getComplexType(structType);
        assertEquals(structType, result);
    }

    @Test
    public void testFilterParameters() throws Exception {
        URL fixtureURL = JoynrGeneratorExtensionsTest.class.getResource("FilterParameters.fidl");
        ModelLoader loader = new ModelLoader(fixtureURL.getPath());
        Resource fixtureResource = loader.getResource(loader.getURIs().iterator().next());
        JoynrGeneratorExtensions extension = mock(JoynrGeneratorExtensions.class, new CallsRealMethods());

        FModel model = (FModel) fixtureResource.getContents().get(0);
        FBroadcast fixture = model.getInterfaces().get(0).getBroadcasts().get(0);

        ArrayList<String> result = extension.getFilterParameters(fixture);
        assertEquals(result.size(), 2);
        assertTrue(result.contains("genre"));
        assertTrue(result.contains("language"));
    }

    @Test
    public void testMultipleOutParameters() throws Exception {
        URL fixtureURL = JoynrGeneratorExtensionsTest.class.getResource("MultipleOutParameters.fidl");
        ModelLoader loader = new ModelLoader(fixtureURL.getPath());
        Resource fixtureResource = loader.getResource(loader.getURIs().iterator().next());
        class MyCallsRealMethods extends CallsRealMethods {
            private static final long serialVersionUID = 1L;

            @Override
            public Object answer(InvocationOnMock invocation) throws Throwable {
                if (invocation.getMethod().getName().equals("getMappedDatatypeOrList")) {
                    Class<?> parameterType0 = invocation.getMethod().getParameterTypes()[0];
                    if (parameterType0.equals(FBasicTypeId.class)) {
                        return ((FBasicTypeId) invocation.getArguments()[0]).getName();
                    } else if (parameterType0.equals(FType.class)) {
                        return ((FType) invocation.getArguments()[0]).getName();
                    } else {
                        return super.answer(invocation);
                    }
                } else {
                    return super.answer(invocation);
                }
            }
        }
        JoynrGeneratorExtensions extension = mock(JoynrGeneratorExtensions.class, new MyCallsRealMethods());

        FModel model = (FModel) fixtureResource.getContents().get(0);
        String stringDatatype = FBasicTypeId.STRING.getName();
        String numberDatatype = FBasicTypeId.INT16.getName();
        String complexDatatype = model.getTypeCollections().get(0).getTypes().get(0).getName();
        FMethod fixture = model.getInterfaces().get(0).getMethods().get(0);

        Iterator<String> result = extension.getMappedOutputParameter(fixture).iterator();
        assertEquals(result.next(), stringDatatype);
        assertEquals(result.next(), numberDatatype);
        assertEquals(result.next(), complexDatatype);
        assertFalse(result.hasNext());
    }
}
