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
package io.joynr.generator.cpp.versioning;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.util.Map;

import org.junit.Test;

import io.joynr.generator.AbstractJoynrCppGeneratorTest;

public class GeneratorVersioningTest extends AbstractJoynrCppGeneratorTest {

    @Test
    public void testGeneratesWithoutVersion() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        final boolean generateVersion = false;
        super.setup(generateProxy, generateProvider, generateVersion);

        boolean versionedStructOfTypeCollectionGenerated = false;
        Map<String, String> result = generate("test-with-noversiongeneration-comment.fidl", true);
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (!entry.getKey().contains("/generated/tests/")
                    && !entry.getKey().contains("/communication-model/generated/")) {
                fail("File generated in wrong package: " + entry.getKey());
            }
            if (entry.getKey().contains("/v2/")) {
                fail("File with package version found: " + entry.getKey());
            }
            if (entry.getKey().contains("NoVersionGenerationTypeCollection/VersionedStruct")) {
                versionedStructOfTypeCollectionGenerated = true;
            }
        }
        assertTrue(versionedStructOfTypeCollectionGenerated);
    }

    @Test
    public void testGeneratesWithVersion() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        final boolean generateVersion = true;
        super.setup(generateProxy, generateProvider, generateVersion);

        boolean versionedStructOfTypeCollectionGenerated = false;
        Map<String, String> result = generate("test-without-noversiongeneration-comment.fidl", true);
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (!entry.getKey().contains("/v2/")) {
                fail("File without package version found: " + entry.getKey());
            }
            if (!entry.getKey().contains("/generated/tests/v2/")
                    && !entry.getKey().contains("/communication-model/generated/datatypes/tests/v2/")
                    && !entry.getKey().contains("/communication-model/generated/interfaces/tests/v2/")) {
                fail("File generated in wrong package: " + entry.getKey());
            }
            if (entry.getKey().contains("PackageVersionedTypeCollection/VersionedStruct")) {
                versionedStructOfTypeCollectionGenerated = true;
            }
        }
        assertTrue(versionedStructOfTypeCollectionGenerated);
    }
}
