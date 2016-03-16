package io.joynr.generator.cpp.communicationmodel.serializer;

import java.net.URL;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FType;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.assistedinject.FactoryModuleBuilder;
import com.google.inject.name.Names;

import io.joynr.generator.cpp.util.CppTemplateFactory;

/*
 * #%L
 * io.joynr.tools.generator:generator-framework
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

import io.joynr.generator.loading.ModelLoader;

public class TypeSerializerTest {

    @Test
    public void testFilterParameters() throws Exception {
        URL fixtureURL = TypeSerializerTest.class.getResource("TypeSerializer.fidl");
        ModelLoader loader = new ModelLoader(fixtureURL.getPath());
        Resource fixtureResource = loader.getResource(loader.getURIs().iterator().next());
        CppTemplateFactory templateFactory = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bindConstant().annotatedWith(Names.named("generationId")).to("");
                install(new FactoryModuleBuilder().build(CppTemplateFactory.class));
            }
        }).getInstance(CppTemplateFactory.class);

        FModel model = (FModel) fixtureResource.getContents().get(0);
        FType fixture = model.getInterfaces().get(0).getTypes().get(0);

        assert fixture instanceof FCompoundType;
        /* this test ensures that no runtime exceptions of the generator occur */
        TypeSerializerCppTemplate typeSerializerCppTemplate = templateFactory.createTypeSerializerCppTemplate((FCompoundType) fixture);
        typeSerializerCppTemplate.generate();
    }

}
