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
package io.joynr.generator;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URISyntaxException;
import java.net.URL;
import java.nio.file.Files;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

import org.eclipse.emf.common.util.URI;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.JavaIoFileSystemAccess;
import org.franca.core.dsl.FrancaIDLStandaloneSetup;
import org.junit.Before;

import com.google.inject.AbstractModule;
import com.google.inject.Inject;
import com.google.inject.Injector;
import com.google.inject.name.Names;

import io.joynr.generator.loading.IUriProvider;
import io.joynr.generator.loading.ModelStore;
import io.joynr.generator.templates.util.JoynrGeneratorExtensions;
import io.joynr.generator.templates.util.NamingUtil;
import io.joynr.generator.templates.util.TypeUtil;
import io.joynr.generator.util.FileSystemAccessUtil;
import io.joynr.generator.util.InvocationArguments;
import io.joynr.generator.cpp.JoynrCppGenerator;

/**
 * Base class for integration testing generation of C++ artifacts from FIDL definitions.
 * Extend this class for your specific tests, then use the {@link #generate(String)}
 * method in order to trigger generation of all artifacts for the specified FIDL
 * file on the classpath (should be put under src/test/resources/io/joynr/generator/cpp).
 */
public abstract class AbstractJoynrCppGeneratorTest {

    private static final Logger logger = Logger.getLogger(AbstractJoynrCppGeneratorTest.class.getName());

    protected Executor executor;
    protected JoynrCppGenerator generator;

    private File temporaryOutputDirectory;

    @Inject
    private IFileSystemAccess outputFileSystem;

    @Before
    public void setup() throws Exception {
        temporaryOutputDirectory = Files.createTempDirectory(null).toFile();
        temporaryOutputDirectory.deleteOnExit();
        InvocationArguments arguments = new InvocationArguments();
        arguments.setGenerationLanguage("cpp");
        arguments.setModelPath("src/test/resources");
        arguments.setOutputPath(temporaryOutputDirectory.getAbsolutePath());

        Injector francaInjector = new FrancaIDLStandaloneSetup().createInjectorAndDoEMFRegistration()
                                                                .createChildInjector(new AbstractModule() {

                                                                    @Override
                                                                    protected void configure() {
                                                                        bindConstant().annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_CLEAN))
                                                                                      .to(false);
                                                                        bindConstant().annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_GENERATE))
                                                                                      .to(true);
                                                                        bindConstant().annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_PACKAGEWITHVERSION))
                                                                                      .to(false);
                                                                        bindConstant().annotatedWith(Names.named(NamingUtil.JOYNR_GENERATOR_NAMEWITHVERSION))
                                                                                      .to(false);
                                                                        bindConstant().annotatedWith(Names.named("generationId"))
                                                                                      .to("5");
                                                                        bind(IFileSystemAccess.class).to(JavaIoFileSystemAccess.class);
                                                                    }
                                                                });
        francaInjector.injectMembers(this);
        generator = new JoynrCppGenerator();
        Injector injector = francaInjector.createChildInjector(generator.getGeneratorModule());
        injector.injectMembers(this);
        injector.injectMembers(generator);
        FileSystemAccessUtil.createFileSystemAccess(outputFileSystem, arguments.getOutputPath());
    }

    /**
     * You can call this method to trigger generation of all Java artifacts for the
     * given FIDL file, where the file must be found on the classpath.
     * The result is a map containing entries keyed by the generated Java filename
     * (without the .java ending) and the value being the content of the
     * generated file.
     *
     * @param fidlFilename the name of the file on the classpath from which to trigger generation.
     *
     * @return a map of generated filename to content of that file.
     */
    protected Map<String, String> generate(String fidlFilename) {
        Map<String, String> result = new HashMap<>();
        URL resourceUrl = AbstractJoynrCppGeneratorTest.class.getClassLoader().getResource(fidlFilename);
        try {
            final URI resourceUri = URI.createFileURI(new File(resourceUrl.toURI()).getAbsolutePath());
            ModelStore modelStore = new ModelStore(new IUriProvider() {

                @Override
                public Iterable<URI> allUris() {
                    return new HashSet<URI>(Arrays.asList(resourceUri));
                }
            });
            generator.doGenerate(modelStore.getResources().iterator().next(), outputFileSystem);
            result = readAllCppFilesRecursively(temporaryOutputDirectory);
        } catch (URISyntaxException e) {
            logger.log(Level.SEVERE, "Problem loading file: " + fidlFilename, e);
        }
        return result;
    }

    private Map<String, String> readAllCppFilesRecursively(File inDirectory) {
        assert inDirectory != null;
        assert inDirectory.isDirectory();
        Map<String, String> result = new HashMap<>();
        for (File file : inDirectory.listFiles()) {
            if (file.isFile() && file.getName().endsWith(".cpp")) {
                result.put(file.getName().replaceAll("\\.cpp$", ""), readContent(file));
            } else if (file.isDirectory()) {
                result.putAll(readAllCppFilesRecursively(file));
            }
        }
        return result;
    }

    private String readContent(File fromFile) {
        StringBuffer buffer = new StringBuffer();
        try (FileInputStream fis = new FileInputStream(fromFile);
                InputStreamReader reader = new InputStreamReader(fis);
                BufferedReader bufferedReader = new BufferedReader(reader)) {
            String line = null;
            while ((line = bufferedReader.readLine()) != null) {
                buffer.append(line).append("\n");
            }
        } catch (IOException e) {
            logger.log(Level.WARNING, "Problem reading result file content from " + fromFile, e);
        }
        return buffer.toString();
    }
}
