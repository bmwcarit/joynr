package io.joynr.generator;

/*
 * #%L
 * joynr::tools::generator::joynr Generator Framework
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
 * %%
 * __________________
 * 
 * NOTICE:  Dissemination of this information or reproduction of this material 
 * is strictly  forbidden unless prior written permission is obtained from 
 * BMW Car IT GmbH.
 * #L%
 */

import io.joynr.generator.loading.ModelLoader;
import io.joynr.generator.util.FrancaIDLFrameworkStandaloneSetup;
import io.joynr.generator.util.InvocationArguments;
import io.joynr.generator.util.TemplatesLoader;

import java.util.logging.Logger;

import javax.inject.Inject;

import org.eclipse.emf.common.util.URI;
import org.eclipse.emf.ecore.resource.Resource;
import org.eclipse.xtext.generator.AbstractFileSystemAccess;
import org.eclipse.xtext.generator.IFileSystemAccess;
import org.eclipse.xtext.generator.IGenerator;

import com.google.inject.AbstractModule;
import com.google.inject.Injector;
import com.google.inject.name.Names;

public class Executor {

    private Logger logger = Logger.getLogger("io.joynr.generator.Executor");
    private IGeneratorWithHeaders headerGenerator = null;
    private IGenerator generator = null;
    private Injector injector;
    private InvocationArguments arguments;

    @Inject
    private IFileSystemAccess outputFileSystem;

    @Inject
    private IFileSystemAccess outputHeaderFileSystem;

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
            }
        });
    }

    protected void setup() throws ClassNotFoundException, InstantiationException, IllegalAccessException {
        String templatesDir = arguments.getTemplatesDir();
        String templatesEncoding = arguments.getTemplatesEncoding();

        if (templatesDir != null) {
            try {
                TemplatesLoader.load(templatesDir, templatesEncoding);
            } catch (Exception e) {
                logger.warning(e.getMessage());
            }
        } else {
            logger.info("skip templates loading, because argument \"templatesDir\" has not been specified");
        }

        String rootGenerator = arguments.getRootGenerator();
        Class<?> rootGeneratorClass = Class.forName(rootGenerator, true, Thread.currentThread().getContextClassLoader());
        Object templateRootInstance = rootGeneratorClass.newInstance();

        // Is this a generator that supports header files?
        if (templateRootInstance instanceof IGeneratorWithHeaders) {
            headerGenerator = (IGeneratorWithHeaders) templateRootInstance;
            generator = (IGenerator) templateRootInstance;
            injector.injectMembers(generator);
        } else if (templateRootInstance instanceof IGenerator) {
            // This is a standard generator
            generator = (IGenerator) templateRootInstance;
            injector.injectMembers(generator);
        } else {
            throw new IllegalStateException("Root generator \"" + "\" is not implementing interface \""
                    + IGenerator.class.getName() + "\"");
        }

    }

    public void execute() {
        // TODO: handle outputHeaderPath
        String outputPath = arguments.getOutputPath();
        String outputHeaderPath = arguments.getOutputHeaderPath();
        String modelPath = arguments.getModelpath();

        createFileSystemAccess(outputFileSystem, outputPath);

        ModelLoader modelLoader = new ModelLoader(modelPath);

        // See if this is the special case where we are using a C++ code generator and need the
        // headers in a separate directory
        if (outputHeaderPath != null && headerGenerator != null) {
            createFileSystemAccess(outputHeaderFileSystem, outputHeaderPath);

            for (URI foundUri : modelLoader.getURIs()) {
                final Resource r = modelLoader.getResource(foundUri);
                headerGenerator.doGenerate(r, outputFileSystem, outputHeaderFileSystem);
            }
        } else {
            // This is a normal generator
            for (URI foundUri : modelLoader.getURIs()) {
                final Resource r = modelLoader.getResource(foundUri);
                generator.doGenerate(r, outputFileSystem);
            }
        }
    }

    protected void createFileSystemAccess(IFileSystemAccess fileSystemAccess, String outputDirectory) {

        if (!(fileSystemAccess instanceof AbstractFileSystemAccess)) {
            throw new IllegalStateException("Guice Module configuration wrong: IFileSystemAccess.class shall be binded to a sub type of org.eclipse.xtext.generator.AbstractFileSystemAccess");
        }
        ((AbstractFileSystemAccess) fileSystemAccess).setOutputPath(outputDirectory);
        ((AbstractFileSystemAccess) fileSystemAccess).getOutputConfigurations()
                                                     .get(IFileSystemAccess.DEFAULT_OUTPUT)
                                                     .setCreateOutputDirectory(true);
    }

}
