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

import io.joynr.generator.js.JoynrJSGenerator;
import io.joynr.generator.loading.IUriProvider;
import io.joynr.generator.loading.ModelStore;
import io.joynr.generator.templates.util.JoynrGeneratorExtensions;
import io.joynr.generator.templates.util.NamingUtil;
import io.joynr.generator.util.FileSystemAccessUtil;
import io.joynr.generator.util.InvocationArguments;

/**
 * Base class for integration testing generation of Js/Ts artifacts from FIDL definitions.
 * Extend this class for your specific tests, then use the {@link #generate(String)}
 * method in order to trigger generation of all artifacts for the specified FIDL
 * file on the classpath (should be put under src/test/resources).
 */
public abstract class AbstractJoynrTSGeneratorTest {

    private static final Logger logger = Logger.getLogger(AbstractJoynrTSGeneratorTest.class.getName());

    protected Executor executor;
    protected JoynrJSGenerator generator;

    private File temporaryOutputDirectory;

    @Inject
    private IFileSystemAccess outputFileSystem;

    public void setup(boolean generateProxy, boolean generateProvider) throws Exception {
        temporaryOutputDirectory = Files.createTempDirectory(null).toFile();
        temporaryOutputDirectory.deleteOnExit();
        InvocationArguments arguments = new InvocationArguments();
        arguments.setGenerationLanguage("javascript");
        arguments.setModelPath("src/test/resources");
        arguments.setOutputPath(temporaryOutputDirectory.getAbsolutePath());
        if (generateProxy && !generateProvider) {
            arguments.setTarget("proxy");
        } else if (!generateProxy && generateProvider) {
            arguments.setTarget("provider");
        }

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
                                                                        bindConstant().annotatedWith(Names.named("generateProxyCode"))
                                                                                      .to(arguments.getGenerateProxyCode());
                                                                        bindConstant().annotatedWith(Names.named("generateProviderCode"))
                                                                                      .to(arguments.getGenerateProviderCode());
                                                                        bind(IFileSystemAccess.class).to(JavaIoFileSystemAccess.class);
                                                                    }
                                                                });
        francaInjector.injectMembers(this);
        generator = new JoynrJSGenerator();
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
        URL resourceUrl = AbstractJoynrTSGeneratorTest.class.getClassLoader().getResource(fidlFilename);
        try {
            final URI resourceUri = URI.createFileURI(new File(resourceUrl.toURI()).getAbsolutePath());
            ModelStore modelStore = new ModelStore(new IUriProvider() {

                @Override
                public Iterable<URI> allUris() {
                    return new HashSet<URI>(Arrays.asList(resourceUri));
                }
            });
            generator.doGenerate(modelStore.getResources().iterator().next(), outputFileSystem);
            result = readAllTSFilesRecursively(temporaryOutputDirectory);
        } catch (URISyntaxException e) {
            logger.log(Level.SEVERE, "Problem loading file: " + fidlFilename, e);
        }
        return result;
    }

    private Map<String, String> readAllTSFilesRecursively(File inDirectory) {
        assert inDirectory != null;
        assert inDirectory.isDirectory();
        Map<String, String> result = new HashMap<>();
        for (File file : inDirectory.listFiles()) {
            if (file.isFile() && file.getName().endsWith(".ts")) {
                result.put(file.getName().replaceAll("\\.ts$", ""), readContent(file));
            } else if (file.isDirectory()) {
                result.putAll(readAllTSFilesRecursively(file));
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
