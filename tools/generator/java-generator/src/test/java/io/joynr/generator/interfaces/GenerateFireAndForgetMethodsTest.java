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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import java.util.Map;
import java.util.logging.Logger;

import org.junit.Before;
import org.junit.Test;

import io.joynr.generator.AbstractJoynrJavaGeneratorTest;

/**
 * Tests that a method with the 'fireAndForget' modifier is generated with
 * the '@FireAndForget' annotation.
 */
public class GenerateFireAndForgetMethodsTest extends AbstractJoynrJavaGeneratorTest {

    @SuppressWarnings("unused")
    private static final Logger logger = Logger.getLogger(GenerateFireAndForgetMethodsTest.class.getName());

    @Before
    public void setup() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        super.setup(generateProxy, generateProvider);
    }

    @Test
    public void testGenerateFireAndForgetMethod() {
        Map<String, String> result = generate("fire-and-forget-test.fidl");
        assertNotNull(result);
        boolean fireAndForgetFound = false;
        boolean syncAsyncFound = false;
        boolean providerFound = false;
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (entry.getKey().endsWith("Sync")
                    || entry.getKey().endsWith("Async") && !entry.getKey().endsWith("StatelessAsync")) {
                assertFalse(entry.getValue().contains("@io.joynr.dispatcher.rpc.annotation.FireAndForget"));
                assertTrue(entry.getValue().contains(", FireAndForgetTestFireAndForget"));
                syncAsyncFound = true;
            } else if (entry.getKey().endsWith("FireAndForget")) {
                assertTrue(entry.getValue().contains("@io.joynr.dispatcher.rpc.annotation.FireAndForget"));
                fireAndForgetFound = true;
            } else if (entry.getKey().equals("FireAndForgetTestProvider")) {
                assertFalse(entry.getValue().contains("Promise"));
                assertFalse(entry.getValue().contains("DeferredVoid"));
                assertTrue(entry.getValue().contains("void callMe"));
                providerFound = true;
            }
        }
        assertTrue(fireAndForgetFound);
        assertTrue(syncAsyncFound);
        assertTrue(providerFound);
    }

    @Test
    public void testDontGenerateFireAndForgetMethod() {
        Map<String, String> result = generate("no-fire-and-forget-test.fidl");
        assertNotNull(result);
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (entry.getKey().endsWith("Sync") || entry.getKey().endsWith("Async")) {
                assertFalse(entry.getValue().contains("@io.joynr.dispatcher.rpc.annotation.FireAndForget"));
            }
        }
    }

    @Test
    public void testGenerateMixedFireAndForgetMethod() {
        Map<String, String> result = generate("fire-and-forget-mixed-test.fidl");
        assertNotNull(result);
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (entry.getKey().endsWith("Sync") || entry.getKey().endsWith("Async")) {
                assertTrue(entry.getValue().contains("callMeTwo("));
                assertFalse(entry.getValue().contains("callMe("));
            } else if (entry.getKey().endsWith("FireAndForget")) {
                assertFalse(entry.getValue().contains("callMeTwo("));
                assertTrue(entry.getValue().contains("callMe("));
            }
        }
    }

    @Test
    public void testGenerateFireAndForgetWithTypes() {
        Map<String, String> result = generate("fire-and-forget-with-types.fidl");
        assertNotNull(result);
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (entry.getKey().endsWith("FireAndForget")) {
                assertTrue(entry.getValue().contains("MyStruct message"));
                assertTrue(entry.getValue().contains("import joynr.fireandforget.MyStruct"));
                assertFalse(entry.getValue().contains("MyOtherStruct otherMessage"));
                assertFalse(entry.getValue().contains("import joynr.fireandforget.MyOtherStruct"));
            } else if (entry.getKey().endsWith("Sync") || entry.getKey().endsWith("Async")) {
                assertFalse(entry.getValue().contains("MyStruct message"));
                assertFalse(entry.getValue().contains("import joynr.fireandforget.MyStruct"));
                assertTrue(entry.getValue().contains("MyOtherStruct otherMessage"));
                assertTrue(entry.getValue().contains("import joynr.fireandforget.MyOtherStruct"));
            } else if (entry.getKey().endsWith(".FireAndForgetProvider")) {
                assertTrue(entry.getValue().contains("MyStruct message"));
                assertTrue(entry.getValue().contains("import joynr.fireandforget.MyStruct"));
                assertTrue(entry.getValue().contains("MyOtherStruct otherMessage"));
                assertTrue(entry.getValue().contains("import joynr.fireandforget.MyOtherStruct"));
            }
        }
    }

}
