/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.generator.cpp;

import static org.junit.Assert.assertTrue;

import java.util.Map;

import org.junit.Test;

import io.joynr.generator.AbstractJoynrCppGeneratorTest;

public class GenerateEnumWithDefaultValueTest extends AbstractJoynrCppGeneratorTest {

    @Test
    public void testGeneratesEnumwithDefaultValue() throws Exception {
        Map<String, String> result = generate("test-struct-with-default-enum-value.fidl");
        TestResult testResult = new TestResult();
        result.forEach((filename, fileContent) -> {
            if (filename.equals("TestStruct")) {
                testResult.setDefaultEnumInitializationFound(fileContent.contains("testEnumerationType(joynr::types::TestEnumerationType::VALUE1)"));
                testResult.setDefaultEnumInitializationWithExplicitValueFound(fileContent.contains("testEnumerationTypeWithExplicitValue(joynr::types::TestEnumerationTypeWithExplicitValue::VALUEWITHDEFAULT1)"));
            }
        });
        assertTrue(testResult.isDefaultEnumInitializationFound());
        assertTrue(testResult.isDefaultEnumInitializationWithExplicitValueFound());
    }

    private static class TestResult {
        boolean defaultEnumInitializationFound;
        boolean defaultEnumInitializationWithExplicitValueFound;

        public boolean isDefaultEnumInitializationFound() {
            return defaultEnumInitializationFound;
        }

        public void setDefaultEnumInitializationFound(boolean defaultEnumInitializationFound) {
            this.defaultEnumInitializationFound = defaultEnumInitializationFound;
        }

        public boolean isDefaultEnumInitializationWithExplicitValueFound() {
            return defaultEnumInitializationWithExplicitValueFound;
        }

        public void setDefaultEnumInitializationWithExplicitValueFound(boolean defaultEnumInitializationWithExplicitValueFound) {
            this.defaultEnumInitializationWithExplicitValueFound = defaultEnumInitializationWithExplicitValueFound;
        }
    }
}
