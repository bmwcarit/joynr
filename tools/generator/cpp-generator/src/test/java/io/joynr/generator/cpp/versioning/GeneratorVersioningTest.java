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

    private void expectVersioning(Map<String, String> result, String typeCollection) {
        boolean versionedStructOfTypeCollectionGenerated = false;
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (!entry.getKey().contains("/v2/")) {
                fail("File without package version found: " + entry.getKey());
            }
            if (!entry.getKey().contains("/generated/tests/v2/")
                    && !entry.getKey().contains("/communication-model/generated/datatypes/tests/v2/")
                    && !entry.getKey().contains("/communication-model/generated/interfaces/tests/v2/")) {
                fail("File generated in wrong package: " + entry.getKey());
            }
            if (entry.getKey().contains(typeCollection + "/VersionedStruct")) {
                versionedStructOfTypeCollectionGenerated = true;
            }
        }
        assertTrue(versionedStructOfTypeCollectionGenerated);
    }

    private void expectNoVersioning(Map<String, String> result, String typeCollection) {
        boolean versionedStructOfTypeCollectionGenerated = false;
        for (Map.Entry<String, String> entry : result.entrySet()) {
            if (!entry.getKey().contains("/generated/tests/")
                    && !entry.getKey().contains("/communication-model/generated/")) {
                fail("File generated in wrong package: " + entry.getKey());
            }
            if (entry.getKey().contains("/v2/")) {
                fail("File with package version found: " + entry.getKey());
            }
            if (entry.getKey().contains(typeCollection + "/VersionedStruct")) {
                versionedStructOfTypeCollectionGenerated = true;
            }
        }
        assertTrue(versionedStructOfTypeCollectionGenerated);
    }

    @Test
    public void generateWithNoVersionComment_withoutAddVersion() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        final boolean packageVersioning = false;
        super.setup(generateProxy, generateProvider, packageVersioning);

        Map<String, String> result = generate("test-with-noversiongeneration-comment.fidl", true);
        expectNoVersioning(result, "NoVersionGenerationTypeCollection");
    }

    @Test
    public void generateWithoutNoVersionComment_withoutAddVersion() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        final boolean packageVersioning = false;
        super.setup(generateProxy, generateProvider, packageVersioning);

        Map<String, String> result = generate("test-without-noversiongeneration-comment.fidl", true);
        expectVersioning(result, "PackageVersionedTypeCollection");
    }

    @Test
    public void generateWithoutNoVersionComment_withAddVersion() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        final boolean packageVersioning = true;
        super.setup(generateProxy, generateProvider, packageVersioning);

        Map<String, String> result = generate("test-without-noversiongeneration-comment.fidl", true);
        expectVersioning(result, "PackageVersionedTypeCollection");
    }

    @Test(expected = IllegalArgumentException.class)
    public void generateWithNoVersionComment_withAddVersion() throws Exception {
        final boolean generateProxy = true;
        final boolean generateProvider = true;
        final boolean packageVersioning = true;
        super.setup(generateProxy, generateProvider, packageVersioning);

        generate("test-with-noversiongeneration-comment.fidl", true);
    }
}
