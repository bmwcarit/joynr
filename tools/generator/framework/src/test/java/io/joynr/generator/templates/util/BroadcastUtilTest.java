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
package io.joynr.generator.templates.util;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import java.net.URL;
import java.util.ArrayList;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FModel;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.name.Names;

import io.joynr.generator.loading.ModelLoader;

public class BroadcastUtilTest {

    @Test
    public void testFilterParameters() throws Exception {
        URL fixtureURL = BroadcastUtilTest.class.getResource("FilterParameters.fidl");
        ModelLoader loader = new ModelLoader(fixtureURL.getPath());
        Resource fixtureResource = loader.getResources().iterator().next();
        BroadcastUtil broadcastUtil = Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(Boolean.class).annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION))
                                   .toInstance(false);
            }
        }).getInstance(BroadcastUtil.class);

        FModel model = (FModel) fixtureResource.getContents().get(0);
        FBroadcast fixture = model.getInterfaces().get(0).getBroadcasts().get(0);

        ArrayList<String> result = broadcastUtil.getFilterParameters(fixture);
        assertEquals(result.size(), 2);
        assertTrue(result.contains("genre"));
        assertTrue(result.contains("language"));
    }

}
