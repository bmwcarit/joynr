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
import static org.mockito.Mockito.mock;
import io.joynr.generator.loading.ModelLoader;

import java.net.URL;
import java.util.ArrayList;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FField;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FTypeRef;
import org.franca.core.franca.FrancaFactory;
import org.junit.Test;
import org.mockito.internal.stubbing.answers.CallsRealMethods;

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

}
