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
package io.joynr.generator.cpp.util;

import java.net.URL;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FInterface;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FStructType;
import org.franca.core.franca.FType;
import org.franca.core.franca.FUnionType;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.assistedinject.FactoryModuleBuilder;
import com.google.inject.name.Names;

import io.joynr.generator.loading.ModelLoader;
import io.joynr.generator.templates.util.JoynrGeneratorExtensions;
import io.joynr.generator.templates.util.NamingUtil;
import junit.framework.TestCase;

public class CppStdTypeUtilTest extends TestCase {

    private CppStdTypeUtil cppStdTypeUtil;
    private FModel model;

    @Override
    protected void setUp() throws Exception {
        URL fixtureURL = CppStdTypeUtilTest.class.getResource("CppStdTypeUtil.fidl");
        ModelLoader loader = new ModelLoader(fixtureURL.getPath());
        Resource fixtureResource = loader.getResources().iterator().next();
        cppStdTypeUtil = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bindConstant().annotatedWith(Names.named("generationId")).to("");
                bindConstant().annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_CLEAN)).to(false);
                bindConstant().annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_GENERATE)).to(true);
                bindConstant().annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION)).to(false);
                install(new FactoryModuleBuilder().build(CppTemplateFactory.class));
            }
        }).getInstance(CppStdTypeUtil.class);

        model = (FModel) fixtureResource.getContents().get(0);
    }

    @Test
    public void testUnionType() throws Exception {
        testGetRootType(model.getInterfaces().get(0), FUnionType.class);
        testGetRootType(model.getInterfaces().get(1), FStructType.class);
    }

    private void testGetRootType(FInterface francaIntf, Class<? extends FCompoundType> type) {
        FType base = francaIntf.getTypes().get(0);
        FType fixture = francaIntf.getTypes().get(1);

        assertTrue(type.isAssignableFrom(base.getClass()));
        assertTrue(type.isAssignableFrom(base.getClass()));
        /* this test ensures that no runtime exceptions of the generator occur */
        assertEquals(base, cppStdTypeUtil.getRootType((FCompoundType) fixture));
        assertEquals(base, cppStdTypeUtil.getRootType((FCompoundType) base));
    }

}
