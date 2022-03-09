/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
import static org.junit.Assert.fail;

import java.util.Map;

import org.junit.Test;

import io.joynr.generator.AbstractJoynrJavaGeneratorTest;

public class ProxyTest extends AbstractJoynrJavaGeneratorTest {

    private final boolean generateProxy = true;
    private final boolean generateProvider = false;
    private final boolean useComment = false;

    private void testOnlyProxyCodeFound(final boolean generateVersion) throws Exception {
        super.setup(generateProxy, generateProvider, generateVersion, useComment);

        Map<String, String> result = generate("multi-out-method-test" + (generateVersion ? "" : "_noversiongeneration")
                + ".fidl");
        assertNotNull(result);
        boolean proxyFound = false;
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (entry.getKey().contains("Provider") || entry.getKey().contains("RequestInterpreter")
                    || entry.getKey().contains("RequestCaller")) {
                fail("Provider related file found");
            }
            if (entry.getKey().contains("Proxy")) {
                proxyFound = true;
            }
        }
        assertTrue("Expected proxy code not found", proxyFound);
    }

    @Test
    public void testOnlyProxyCodeFound_withVersioning() throws Exception {
        testOnlyProxyCodeFound(true);
    }

    @Test
    public void testOnlyProxyCodeFound_noVersioning() throws Exception {
        testOnlyProxyCodeFound(false);
    }
}
