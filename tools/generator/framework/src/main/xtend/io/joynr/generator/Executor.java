package io.joynr.generator;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import static io.joynr.generator.util.FileSystemAccessUtil.createFileSystemAccess;
import io.joynr.generator.loading.ModelLoader;
import io.joynr.generator.util.FrancaIDLFrameworkStandaloneSetup;
import io.joynr.generator.util.InvocationArguments;
import io.joynr.generator.util.JoynrGeneratorExtensions;
import io.joynr.generator.util.TemplatesLoader;

import java.util.logging.Logger;

import javax.inject.Inject;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.IGenerator;

import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.name.Names;

public class Executor {

    private Logger logger = Logger.getLogger("io.joynr.generator.Executor");
    private Injector injector;
    private InvocationArguments arguments;

    @Inject
    private IFileSystemAccess outputFileSystem;

    public Executor(final InvocationArguments arguments) {
        this.arguments = arguments;

        // Get an injector and inject into the current instance
        Injector francaInjector = new FrancaIDLFrameworkStandaloneSetup().createInjectorAndDoEMFRegistration();
        francaInjector.injectMembers(this);

        // Use a child injector that contains configuration parameters passed to this Executor
        this.injector = francaInjector.createChildInjector(new AbstractModule() {
            @Override
            protected void configure() {
                String generationId = arguments.getGenerationId();
                if (generationId != null) {
                    bindConstant().annotatedWith(Names.named("generationId")).to(generationId);
                } else {
                    // Guice does not allow null binding - use an empty string to show there is no generationId
                    bindConstant().annotatedWith(Names.named("generationId")).to("");
                }
                bind(Boolean.class).annotatedWith(Names.named(JoynrGeneratorExtensions.JOYNR_GENERATOR_GENERATE))
                                   .toInstance(arguments.shallGenerate());
            }
        });
    }

    protected IGenerator setup() throws ClassNotFoundException, InstantiationException, IllegalAccessException {
        String templatesDir = arguments.getTemplatesDir();
        String templatesEncoding = arguments.getTemplatesEncoding();

        if (templatesDir != null) {
            try {
                TemplatesLoader.load(templatesDir, templatesEncoding);
            } catch (Exception e) {
                logger.warning(e.getMessage());
            }
        }

        String rootGenerator = arguments.getRootGenerator();
        Class<?> rootGeneratorClass = Class.forName(rootGenerator, true, Thread.currentThread().getContextClassLoader());
        Object templateRootInstance = rootGeneratorClass.newInstance();

        // Is this a generator that supports header files?
        if (templateRootInstance instanceof IGenerator) {
            // This is a standard generator
            IGenerator result = (IGenerator) templateRootInstance;
            injector.injectMembers(result);
            return result;
        } else {
            throw new IllegalStateException("Root generator \"" + "\" is not implementing interface \""
                    + IGenerator.class.getName() + "\"");
        }

    }

    private ModelLoader prepareGeneratorEnvironment(IGenerator generator) {
        String outputPath = arguments.getOutputPath();
        String modelPath = arguments.getModelpath();

        createFileSystemAccess(outputFileSystem, outputPath);

        // This is a normal generator
        if (generator instanceof IJoynrGenerator) {
            ((IJoynrGenerator) generator).setParameters(arguments.getParameter());
        }

        return new ModelLoader(modelPath);
    }

    public void generate(IGenerator generator) {
        ModelLoader modelLoader = prepareGeneratorEnvironment(generator);
        for (URI foundUri : modelLoader.getURIs()) {
            final Resource r = modelLoader.getResource(foundUri);
            generator.doGenerate(r, outputFileSystem);
        }
    }
}
