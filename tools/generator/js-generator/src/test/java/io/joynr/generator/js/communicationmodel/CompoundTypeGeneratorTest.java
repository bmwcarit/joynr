/*
 * #%L
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
package io.joynr.generator.js.communicationmodel;

import org.franca.core.franca.FField;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FTypeCollection;
import org.franca.core.franca.FTypeRef;
import org.franca.core.franca.FrancaFactory;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.assistedinject.FactoryModuleBuilder;
import com.google.inject.name.Names;

import io.joynr.generator.js.util.JsTemplateFactory;
import io.joynr.generator.templates.util.JoynrGeneratorExtensions;
import io.joynr.generator.templates.util.NamingUtil;

public class CompoundTypeGeneratorTest {

    @Test
    public void testRecursiveStruct() throws Exception {
        FModel model = FrancaFactory.eINSTANCE.createFModel();
        FStructType structType = FrancaFactory.eINSTANCE.createFStructType();
        FTypeCollection typeCollection = FrancaFactory.eINSTANCE.createFTypeCollection();
        typeCollection.getTypes().add(structType);
        model.getTypeCollections().add(typeCollection);
        structType.setName("TestStruct");
        FField field = FrancaFactory.eINSTANCE.createFField();
        field.setName("exampleField");
        field.setArray(true);
        FTypeRef typeRef = FrancaFactory.eINSTANCE.createFTypeRef();
        typeRef.setDerived(structType);
        field.setType(typeRef);
        structType.getElements().add(field);

        JsTemplateFactory templateFactory = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                install(new FactoryModuleBuilder().build(JsTemplateFactory.class));
                bind(Boolean.class).annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_GENERATE))
                                   .toInstance(true);
                bind(Boolean.class).annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_CLEAN))
                                   .toInstance(false);
                bind(Boolean.class).annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION))
                                   .toInstance(false);
            }
        }).getInstance(JsTemplateFactory.class);
        CompoundTypeGenerator generator = templateFactory.createCompoundTypeGenerator(structType);
        generator.generate();
    }

}
