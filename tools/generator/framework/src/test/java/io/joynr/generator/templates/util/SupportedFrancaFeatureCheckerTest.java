package io.joynr.generator.templates.util;

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

import java.net.URL;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FModel;
import org.junit.Test;

import io.joynr.generator.loading.ModelLoader;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class SupportedFrancaFeatureCheckerTest {

    @Test
    public void testIntegerTypeIsUsed_ExceptionIsThrown() {
        URL fixtureURL = SupportedFrancaFeatureCheckerTest.class.getResource("IntegerTypeUsed.fidl");
        ModelLoader loader = new ModelLoader(fixtureURL.getPath());
        Resource fixtureResource = loader.getResources().iterator().next();

        FModel model = (FModel) fixtureResource.getContents().get(0);

        try {
            SupportedFrancaFeatureChecker.checkModel(model);
            fail("This line should not be reached, as an exception is expected when invoking checkModel");
        } catch (Exception e) {
            assertTrue(e.getMessage().contains("not supported"));
        }
    }
}
