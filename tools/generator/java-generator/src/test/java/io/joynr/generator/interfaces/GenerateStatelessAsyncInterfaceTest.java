/*-
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

import static org.junit.Assert.assertTrue;

import java.util.Map;

import org.junit.Test;

import io.joynr.generator.AbstractJoynrJavaGeneratorTest;

public class GenerateStatelessAsyncInterfaceTest extends AbstractJoynrJavaGeneratorTest {

    private final boolean generateProxy = true;
    private final boolean generateProvider = true;
    private final boolean useComment = false;

    private void testGeneratesStatelessAsyncInterfaceWithAllMethods(final boolean generateVersion) throws Exception {
        super.setup(generateProxy, generateProvider, generateVersion, useComment);

        Map<String, String> result = generate("stateless-async-test" + (generateVersion ? "" : "_noversiongeneration")
                + ".fidl");
        TestResult testResult = new TestResult();
        result.forEach((filename, fileContent) -> {
            if (filename.endsWith("StatelessAsync")) {
                // Comment in to manually inspect the generator output during testing
                //System.out.println("Stateless async interface:\n" + fileContent);
                testResult.setStatelessAsyncInterfaceFound(true);
                testResult.setStatelessAsyncAnnotationAdded(fileContent.contains("import io.joynr.StatelessAsync;")
                        && fileContent.contains("@StatelessAsync"));
                testResult.setExtendsFireAndForgetFound(fileContent.contains(", StatelessAsyncTestFireAndForget")
                        && !fileContent.contains("callFireAndForget"));
                testResult.setReadWriteAttributeFound(fileContent.contains("getTestAttribute(")
                        && fileContent.contains("setTestAttribute("));
                testResult.setReadOnlyAttributeFound(fileContent.contains("getTestReadOnlyAttribute(")
                        && !fileContent.contains("setTestReadOnlyAttribute("));
                testResult.setNoOutMethodFound(fileContent.contains("void noOutMethod(")
                        && fileContent.contains("String noOutInData") && fileContent.contains("noOutInDataOne")
                        && fileContent.contains("noOutInDataTwo"));
                testResult.setNoInOneOutMethodFound(fileContent.contains("void noInOneOutMethod(")
                        && !fileContent.contains("String noInOutData"));
                testResult.setOneInOneOutMethodFound(fileContent.contains("void oneInOneOutMethod(")
                        && fileContent.contains("String oneInData"));
                testResult.setTwoInOneOutMethodFound(fileContent.contains("void twoInOneOutMethod(")
                        && fileContent.contains("String twoInDataOne") && fileContent.contains("String twoInDataTwo"));
                testResult.setOneInTwoOutMethodFound(fileContent.contains("void oneInTwoOutMethod(")
                        && fileContent.contains("String threeInData"));
                testResult.setWithErrorMethodFound(fileContent.contains("void withError(")
                        && fileContent.contains("String inData"));
                testResult.setTestTypeInputImportFound(fileContent.contains("import joynr.statelessasync"
                        + (generateVersion ? ".v0" : "") + ".testTypeCollection.TestTypeInput;"));
                testResult.setTestTypeOutputImportNotFound(!fileContent.contains("import joynr.statelessasync"
                        + (generateVersion ? ".v0" : "") + ".testTypeCollection.TestTypeOutput;"));
            }
        });
        assertTrue(testResult.isStatelessAsyncInterfaceFound());
        assertTrue(testResult.isStatelessAsyncAnnotationAdded());
        assertTrue(testResult.isExtendsFireAndForgetFound());
        assertTrue(testResult.isReadWriteAttributeFound());
        assertTrue(testResult.isReadOnlyAttributeFound());
        assertTrue(testResult.isNoOutMethodFound());
        assertTrue(testResult.isNoInOneOutMethodFound());
        assertTrue(testResult.isOneInOneOutMethodFound());
        assertTrue(testResult.isTwoInOneOutMethodFound());
        assertTrue(testResult.isOneInTwoOutMethodFound());
        assertTrue(testResult.isWithErrorMethodFound());
        assertTrue(testResult.isTestTypeInputImportFound());
        assertTrue(testResult.isTestTypeOutputImportNotFound());
    }

    @Test
    public void testGeneratesStatelessAsyncInterfaceWithAllMethods_withVersioning() throws Exception {
        testGeneratesStatelessAsyncInterfaceWithAllMethods(true);
    }

    @Test
    public void testGeneratesStatelessAsyncInterfaceWithAllMethods_noVersioning() throws Exception {
        testGeneratesStatelessAsyncInterfaceWithAllMethods(false);
    }

    private static class TestResult {
        boolean statelessAsyncInterfaceFound;
        boolean statelessAsyncAnnotationAdded;
        boolean extendsFireAndForgetFound;
        boolean readWriteAttributeFound;
        boolean readOnlyAttributeFound;
        boolean noOutMethodFound;
        boolean noInOneOutMethodFound;
        boolean oneInOneOutMethodFound;
        boolean twoInOneOutMethodFound;
        boolean oneInTwoOutMethodFound;
        boolean withErrorMethodFound;
        boolean testTypeInputImportFound;
        boolean testTypeOutputImportNotFound;

        public boolean isStatelessAsyncInterfaceFound() {
            return statelessAsyncInterfaceFound;
        }

        public void setStatelessAsyncInterfaceFound(boolean statelessAsyncInterfaceFound) {
            this.statelessAsyncInterfaceFound = statelessAsyncInterfaceFound;
        }

        public boolean isStatelessAsyncAnnotationAdded() {
            return statelessAsyncAnnotationAdded;
        }

        public void setStatelessAsyncAnnotationAdded(boolean statelessAsyncAnnotationAdded) {
            this.statelessAsyncAnnotationAdded = statelessAsyncAnnotationAdded;
        }

        public boolean isExtendsFireAndForgetFound() {
            return extendsFireAndForgetFound;
        }

        public void setExtendsFireAndForgetFound(boolean extendsFireAndForgetFound) {
            this.extendsFireAndForgetFound = extendsFireAndForgetFound;
        }

        public boolean isNoOutMethodFound() {
            return noOutMethodFound;
        }

        public void setNoOutMethodFound(boolean noOutMethodFound) {
            this.noOutMethodFound = noOutMethodFound;
        }

        public boolean isNoInOneOutMethodFound() {
            return noInOneOutMethodFound;
        }

        public void setNoInOneOutMethodFound(boolean noInOneOutMethodFound) {
            this.noInOneOutMethodFound = noInOneOutMethodFound;
        }

        public boolean isOneInOneOutMethodFound() {
            return oneInOneOutMethodFound;
        }

        public void setOneInOneOutMethodFound(boolean oneInOneOutMethodFound) {
            this.oneInOneOutMethodFound = oneInOneOutMethodFound;
        }

        public boolean isOneInTwoOutMethodFound() {
            return oneInTwoOutMethodFound;
        }

        public void setOneInTwoOutMethodFound(boolean oneInTwoOutMethodFound) {
            this.oneInTwoOutMethodFound = oneInTwoOutMethodFound;
        }

        public boolean isTwoInOneOutMethodFound() {
            return twoInOneOutMethodFound;
        }

        public void setTwoInOneOutMethodFound(boolean twoInOneOutMethodFound) {
            this.twoInOneOutMethodFound = twoInOneOutMethodFound;
        }

        public boolean isWithErrorMethodFound() {
            return withErrorMethodFound;
        }

        public void setWithErrorMethodFound(boolean withErrorMethodFound) {
            this.withErrorMethodFound = withErrorMethodFound;
        }

        public boolean isReadWriteAttributeFound() {
            return readWriteAttributeFound;
        }

        public void setReadWriteAttributeFound(boolean readWriteAttributeFound) {
            this.readWriteAttributeFound = readWriteAttributeFound;
        }

        public boolean isReadOnlyAttributeFound() {
            return readOnlyAttributeFound;
        }

        public void setReadOnlyAttributeFound(boolean readOnlyAttributeFound) {
            this.readOnlyAttributeFound = readOnlyAttributeFound;
        }

        public boolean isTestTypeInputImportFound() {
            return testTypeInputImportFound;
        }

        public void setTestTypeInputImportFound(boolean testTypeInputImportFound) {
            this.testTypeInputImportFound = testTypeInputImportFound;
        }

        public boolean isTestTypeOutputImportNotFound() {
            return testTypeOutputImportNotFound;
        }

        public void setTestTypeOutputImportNotFound(boolean testTypeOutputImportNotFound) {
            this.testTypeOutputImportNotFound = testTypeOutputImportNotFound;
        }
    }
}
