package io.joynr.generator.interfaces;

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

import static org.junit.Assert.assertTrue;

import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.junit.Test;

import io.joynr.generator.AbstractJoynrJavaGeneratorTest;

public class GenerateStatelessAsyncCallbackInterfaceTest extends AbstractJoynrJavaGeneratorTest {

    private final boolean generateProxy = true;
    private final boolean generateProvider = true;
    private final boolean useComment = false;

    private void testGeneratesStatelessAsyncCallbackInterfaceWithAllMethods(final boolean generateVersion) throws Exception {
        super.setup(generateProxy, generateProvider, generateVersion, useComment);

        Map<String, String> result = generate("stateless-async-test" + (generateVersion ? "" : "_noversiongeneration")
                + ".fidl");
        TestResult testResult = new TestResult();
        Pattern noOutMethodPattern = Pattern.compile("void noOutMethodSuccess\\(");
        result.forEach((filename, fileContent) -> {
            if (filename.endsWith("StatelessAsyncCallback")) {
                // Comment in to manually inspect the generator output during testing
                // System.out.println("Stateless async callback interface:\n" + fileContent);
                testResult.setStatelessAsyncCallbackInterfaceFound(true);
                testResult.setExtendsStatelessAsyncCallback(fileContent.contains("import io.joynr.proxy.StatelessAsyncCallback;")
                        && fileContent.contains("extends StatelessAsyncCallback"));
                testResult.setReadWriteAttributeFound(fileContent.contains("getTestAttributeSuccess")
                        && fileContent.contains("setTestAttributeSuccess"));
                testResult.setReadOnlyAttributeFound(fileContent.contains("getTestReadOnlyAttributeSuccess")
                        && !fileContent.contains("setTestReadOnlyAttributeSuccess"));
                Matcher noOutMethodMatcher = noOutMethodPattern.matcher(fileContent);
                testResult.setNoOutMethodFound(noOutMethodMatcher.find()
                        && !noOutMethodMatcher.find(noOutMethodMatcher.end()));
                testResult.setNoInOneOutMethodFound(fileContent.contains("default void noInOneOutMethodSuccess(")
                        && fileContent.contains("String noInOutData"));
                testResult.setOneInOneOutMethodFound(fileContent.contains("default void oneInOneOutMethodSuccess(")
                        && fileContent.contains("String oneOutData"));
                testResult.setTwoInOneOutMethodFound(fileContent.contains("default void twoInOneOutMethodSuccess(")
                        && fileContent.contains("String twoOutData"));
                testResult.setOneInTwoOutMethodFound(fileContent.contains("default void oneInOneOutMethodSuccess(")
                        && fileContent.contains("String threeOutDataOne")
                        && fileContent.contains("String threeOutDataTwo"));
                testResult.setWithErrorMethodFound(fileContent.contains("default void withErrorSuccess(")
                        && fileContent.contains("String outData"));
                testResult.setWithErrorMethodFailedFound(fileContent.contains("default void withErrorFailed(")
                        && fileContent.contains("joynr.statelessasync" + (generateVersion ? ".v0" : "")
                                + ".StatelessAsyncTest.WithErrorErrorEnum error,"));
                testResult.setTestTypeInputImportNotFound(!fileContent.contains("import joynr.statelessasync"
                        + (generateVersion ? ".v0" : "") + ".testTypeCollection.TestTypeInput;"));
                testResult.setTestTypeOutputImportFound(fileContent.contains("import joynr.statelessasync"
                        + (generateVersion ? ".v0" : "") + ".testTypeCollection.TestTypeOutput;"));
            }
        });
        assertTrue(testResult.isStatelessAsyncCallbackInterfaceFound());
        assertTrue(testResult.isExtendsStatelessAsyncCallback());
        assertTrue(testResult.isReadWriteAttributeFound());
        assertTrue(testResult.isReadOnlyAttributeFound());
        assertTrue(testResult.isNoOutMethodFound());
        assertTrue(testResult.isNoInOneOutMethodFound());
        assertTrue(testResult.isOneInOneOutMethodFound());
        assertTrue(testResult.isTwoInOneOutMethodFound());
        assertTrue(testResult.isOneInTwoOutMethodFound());
        assertTrue(testResult.isWithErrorMethodFound());
        assertTrue(testResult.isWithErrorMethodFailedFound());
        assertTrue(testResult.isTestTypeInputImportNotFound());
        assertTrue(testResult.isTestTypeOutputImportFound());
    }

    @Test
    public void testGeneratesStatelessAsyncCallbackInterfaceWithAllMethods_withVersioning() throws Exception {
        testGeneratesStatelessAsyncCallbackInterfaceWithAllMethods(true);
    }

    @Test
    public void testGeneratesStatelessAsyncCallbackInterfaceWithAllMethods_noVersioning() throws Exception {
        testGeneratesStatelessAsyncCallbackInterfaceWithAllMethods(false);
    }

    private static class TestResult {
        boolean statelessAsyncCallbackInterfaceFound;
        boolean extendsStatelessAsyncCallback;
        boolean readWriteAttributeFound;
        boolean readOnlyAttributeFound;
        boolean noOutMethodFound;
        boolean noInOneOutMethodFound;
        boolean oneInOneOutMethodFound;
        boolean twoInOneOutMethodFound;
        boolean oneInTwoOutMethodFound;
        boolean withErrorMethodFound;
        boolean withErrorMethodFailedFound;
        boolean testTypeInputImportNotFound;
        boolean testTypeOutputImportFound;

        public boolean isStatelessAsyncCallbackInterfaceFound() {
            return statelessAsyncCallbackInterfaceFound;
        }

        public void setStatelessAsyncCallbackInterfaceFound(boolean statelessAsyncCallbackInterfaceFound) {
            this.statelessAsyncCallbackInterfaceFound = statelessAsyncCallbackInterfaceFound;
        }

        public boolean isExtendsStatelessAsyncCallback() {
            return extendsStatelessAsyncCallback;
        }

        public void setExtendsStatelessAsyncCallback(boolean extendsStatelessAsyncCallback) {
            this.extendsStatelessAsyncCallback = extendsStatelessAsyncCallback;
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

        public boolean isWithErrorMethodFailedFound() {
            return withErrorMethodFailedFound;
        }

        public void setWithErrorMethodFailedFound(boolean withErrorMethodFailedFound) {
            this.withErrorMethodFailedFound = withErrorMethodFailedFound;
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

        public boolean isTestTypeInputImportNotFound() {
            return testTypeInputImportNotFound;
        }

        public void setTestTypeInputImportNotFound(boolean testTypeInputImportNotFound) {
            this.testTypeInputImportNotFound = testTypeInputImportNotFound;
        }

        public boolean isTestTypeOutputImportFound() {
            return testTypeOutputImportFound;
        }

        public void setTestTypeOutputImportFound(boolean testTypeOutputImportFound) {
            this.testTypeOutputImportFound = testTypeOutputImportFound;
        }
    }
}
