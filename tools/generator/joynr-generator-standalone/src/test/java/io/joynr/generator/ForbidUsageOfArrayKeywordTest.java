package io.joynr.generator;
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

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.File;
import java.nio.file.Files;

import org.junit.Before;
import org.junit.Test;

import io.joynr.generator.util.InvocationArguments;

public class ForbidUsageOfArrayKeywordTest {

    private File temporaryOutputDirectory;
    private InvocationArguments invocationArguments;
    Executor testExector;

    @Before
    public void setup() throws Exception {

        temporaryOutputDirectory = Files.createTempDirectory(null).toFile();
        temporaryOutputDirectory.deleteOnExit();
        invocationArguments = new InvocationArguments();
        invocationArguments.setGenerate(true);
        invocationArguments.setOutputPath(temporaryOutputDirectory.getAbsolutePath());
    }

    private void createExecutor() throws Exception {
        testExector = new Executor(invocationArguments);
    }

    @Test
    public void generateCppWithArrayKeywordInTypeCollectionThrows() throws Exception {
        invocationArguments.setGenerationLanguage("cpp");
        invocationArguments.setModelPath("src/test/resources/test-explicit-array-in-typecollection.fidl");
        createExecutor();
        try {
            testExector.generate();
            fail("expected to throw");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains("array NAME of TYPE"));
        }
    }

    @Test
    public void generateCppWithArrayKeywordInterfaceThrows() throws Exception {
        invocationArguments.setGenerationLanguage("cpp");
        invocationArguments.setModelPath("src/test/resources/test-explicit-array-in-interface.fidl");
        createExecutor();
        try {
            testExector.generate();
            fail("expected to throw");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains("array NAME of TYPE"));
        }
    }

    @Test
    public void generateCppWithImplicitArraysDoesntThrow() throws Exception {
        invocationArguments.setGenerationLanguage("cpp");
        invocationArguments.setModelPath("src/test/resources/test-implicit-arrays.fidl");
        createExecutor();
        testExector.generate();
    }

    @Test
    public void generateJavaWithArrayKeywordInTypeCollectionThrows() throws Exception {
        invocationArguments.setGenerationLanguage("java");
        invocationArguments.setModelPath("src/test/resources/test-explicit-array-in-typecollection.fidl");
        createExecutor();
        try {
            testExector.generate();
            fail("expected to throw");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains("array NAME of TYPE"));
        }
    }

    @Test
    public void generateJavaWithArrayKeywordInterfaceThrows() throws Exception {
        invocationArguments.setGenerationLanguage("java");
        invocationArguments.setModelPath("src/test/resources/test-explicit-array-in-interface.fidl");
        createExecutor();
        try {
            testExector.generate();
            fail("expected to throw");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains("array NAME of TYPE"));
        }
    }

    @Test
    public void generateJavaWithImplicitArraysDoesntThrow() throws Exception {
        invocationArguments.setGenerationLanguage("java");
        invocationArguments.setModelPath("src/test/resources/test-implicit-arrays.fidl");
        createExecutor();
        testExector.generate();
    }

    @Test
    public void generateJsWithArrayKeywordInTypeCollectionThrows() throws Exception {
        invocationArguments.setGenerationLanguage("javascript");
        invocationArguments.setModelPath("src/test/resources/test-explicit-array-in-typecollection.fidl");
        createExecutor();
        try {
            testExector.generate();
            fail("expected to throw");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains("array NAME of TYPE"));
        }
    }

    @Test
    public void generateJsWithArrayKeywordInterfaceThrows() throws Exception {
        invocationArguments.setGenerationLanguage("javascript");
        invocationArguments.setModelPath("src/test/resources/test-explicit-array-in-interface.fidl");
        createExecutor();
        try {
            testExector.generate();
            fail("expected to throw");
        } catch (IllegalArgumentException e) {
            assertTrue(e.getMessage().contains("array NAME of TYPE"));
        }
    }

    @Test
    public void generateJsWithImplicitArraysDoesntThrow() throws Exception {
        invocationArguments.setGenerationLanguage("javascript");
        invocationArguments.setModelPath("src/test/resources/test-implicit-arrays.fidl");
        createExecutor();
        testExector.generate();
    }

}
