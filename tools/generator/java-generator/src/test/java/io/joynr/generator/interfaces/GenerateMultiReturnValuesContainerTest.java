/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
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
package io.joynr.generator.interfaces;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.util.Map;

import org.junit.Test;

import io.joynr.generator.AbstractJoynrJavaGeneratorTest;

public class GenerateMultiReturnValuesContainerTest extends AbstractJoynrJavaGeneratorTest {

    private final boolean generateProxy = true;
    private final boolean generateProvider = true;
    private final boolean useComment = false;

    private void testGenerateMultiReturnMethod(final boolean generateVersion) throws Exception {
        super.setup(generateProxy, generateProvider, generateVersion, useComment);

        Map<String, String> result = generate("multi-out-method-test" + (generateVersion ? "" : "_noversiongeneration")
                + ".fidl");
        assertNotNull(result);
        boolean containerFound = false;
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (entry.getKey().endsWith("Sync")) {
                assertTrue("Marker interface not added.",
                           entry.getValue().contains("implements io.joynr.dispatcher.rpc.MultiReturnValuesContainer"));
                containerFound = true;
            }
        }
        assertTrue(containerFound);
    }

    @Test
    public void testGenerateMultiReturnMethod_withVersioning() throws Exception {
        testGenerateMultiReturnMethod(true);
    }

    @Test
    public void testGenerateMultiReturnMethod_noVersioning() throws Exception {
        testGenerateMultiReturnMethod(false);
    }

}
