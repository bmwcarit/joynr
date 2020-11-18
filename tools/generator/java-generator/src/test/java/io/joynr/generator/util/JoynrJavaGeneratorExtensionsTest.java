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
package io.joynr.generator.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Iterator;

import org.eclipse.emf.common.util.EList;
import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FCompoundType;
import org.franca.core.franca.FModel;
import org.franca.core.franca.FType;
import org.franca.core.franca.FTypeCollection;
import org.junit.Before;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.generator.loading.ModelLoader;
import io.joynr.generator.templates.util.JoynrGeneratorExtensions;
import io.joynr.generator.templates.util.NamingUtil;
import io.joynr.generator.util.JoynrJavaGeneratorExtensions;

public class JoynrJavaGeneratorExtensionsTest {

    private static final String SECOND_LEVEL_STRUCT_TYPE_NAME = "joynr.io.joynr.generator.util.TestTypes.SecondLevelStruct";
    private static final String TOP_LEVEL_STRUCT_INCLUDE = "joynr.io.joynr.generator.util.TestTypes.TopLevelStruct";
    private JoynrJavaGeneratorExtensions fixture;
    private static final String FIXTURE_TYPE_NAME = "fixture";

    @Before
    public void setup() {
        Injector injector = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(Boolean.class).annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_GENERATE))
                                   .toInstance(true);
                bind(Boolean.class).annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_CLEAN))
                                   .toInstance(false);
                bind(Boolean.class).annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION))
                                   .toInstance(false);
            }
        });
        fixture = injector.getInstance(JoynrJavaGeneratorExtensions.class);
    }

    @Test
    public void testHierarchicalStructWithTwoLevelWithoutMembers() {
        testIncludesOfFixture(false, true, "JoynrJavaGeneratorExtensionsTestWithoutMembers.fidl");
    }

    @Test
    public void testHierarchicalStructWithTwoLevelWithMembers() {
        testIncludesOfFixture(true, true, "JoynrJavaGeneratorExtensionsTestWithMembers.fidl");
    }

    private void testIncludesOfFixture(boolean shallIncludeTopLevel, boolean shallIncludeSecondLevel, String model) {
        // this test shall check for correct import statements (avoiding
        // warnings) in case of hierachical structs
        ModelLoader modelLoader = new ModelLoader(model);
        Iterator<Resource> modelResourcesIterator = modelLoader.getResources().iterator();
        Resource input = modelResourcesIterator.next();
        assertFalse(modelResourcesIterator.hasNext());

        FModel fModel = (FModel) input.getContents().get(0);

        EList<FTypeCollection> typeCollections = fModel.getTypeCollections();

        assertEquals(1, typeCollections.size());

        boolean typeFound = false;
        for (FType type : typeCollections.get(0).getTypes()) {
            if (type.getName().equals(FIXTURE_TYPE_NAME)) {
                if (type instanceof FCompoundType) {
                    typeFound = true;
                    Iterable<String> result = fixture.getRequiredIncludesFor((FCompoundType) type, true);

                    boolean topLevelStructAsInclude = false;
                    boolean secondLevelStructAsInclude = false;
                    for (String include : result) {
                        if (include.equals(TOP_LEVEL_STRUCT_INCLUDE)) {
                            topLevelStructAsInclude = true;
                        } else if (include.equals(SECOND_LEVEL_STRUCT_TYPE_NAME)) {
                            secondLevelStructAsInclude = true;
                        }
                    }
                    assertEquals(TOP_LEVEL_STRUCT_INCLUDE + " shall NOT be part of includes",
                                 shallIncludeTopLevel,
                                 topLevelStructAsInclude);
                    assertEquals(SECOND_LEVEL_STRUCT_TYPE_NAME + " shall be part of includes",
                                 shallIncludeSecondLevel,
                                 secondLevelStructAsInclude);
                }
            }
        }
        assertTrue(typeFound);
    }

}
