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
import io.joynr.generator.loading.ModelLoader;

import java.net.URL;
import java.util.ArrayList;

import org.eclipse.emf.ecore.resource.Resource;
import org.franca.core.franca.FArgument;
import org.franca.core.franca.FBroadcast;
import org.franca.core.franca.FModel;
import org.junit.Before;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.name.Names;

public class BroadcastUtilTest {

    private BroadcastUtil broadcastUtil;
    private FModel model;

    @Before
    public void setUp() {
        broadcastUtil = Guice.createInjector(new AbstractModule() {
            @Override
            protected void configure() {
                bindConstant().annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_INTERFACENAMEWITHVERSION))
                              .to(false);
            }
        }).getInstance(BroadcastUtil.class);

        URL fidlURL = BroadcastUtilTest.class.getResource("SelectiveBroadcastTest.fidl");
        ModelLoader loader = new ModelLoader(fidlURL.getPath());
        Resource fidlResource = loader.getResources().iterator().next();
        model = (FModel) fidlResource.getContents().get(0);
    }

    @Test
    public void testFilterParameters() {
        FBroadcast fixture = model.getInterfaces().get(0).getBroadcasts().get(0);

        assertEquals("fixture", fixture.getName());
        ArrayList<String> result = broadcastUtil.getFilterParameters(fixture);
        assertEquals(result.size(), 2);
        assertTrue(result.contains("genre"));
        assertTrue(result.contains("language"));
    }

    @Test
    public void testOutputParameters() {
        FBroadcast fixture = model.getInterfaces().get(0).getBroadcasts().get(0);

        assertEquals("fixture", fixture.getName());
        Iterable<FArgument> result = broadcastUtil.getOutputParameters(fixture);
        assertTrue(result.iterator().hasNext());
        assertEquals("station", result.iterator().next().getName());
    }

    @Test(expected = IllegalStateException.class)
    public void testEmptyOutputParametersIsNotSupportedForSelectiveBroadcast() {
        FBroadcast emptyOutput = model.getInterfaces().get(0).getBroadcasts().get(1);

        assertEquals("emptyOutput", emptyOutput.getName());
        broadcastUtil.getOutputParameters(emptyOutput);
    }

    @Test(expected = IllegalStateException.class)
    public void testNoOutputParametersIsNotSupportedForSelectiveBroadcast() {
        FBroadcast noOutput = model.getInterfaces().get(0).getBroadcasts().get(2);

        assertEquals("noOutput", noOutput.getName());
        broadcastUtil.getOutputParameters(noOutput);
    }
}
