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

import io.joynr.generator.util.InvocationArguments;

import org.eclipse.equinox.app.IApplication;
import org.eclipse.equinox.app.IApplicationContext;

public class GeneratorApplication implements IApplication {

    public Object start(IApplicationContext context) throws Exception {
        System.err.println("Generation started ...");

        InvocationArguments iA = new InvocationArguments((String[]) context.getArguments().get("application.args"));
        System.err.println("Invokation Parameters: ");
        System.err.println("	modelPath: " + iA.getModelpath());
        System.err.println("	templatesDir: " + iA.getTemplatesDir());
        System.err.println("	templatesEncoding: " + iA.getTemplatesEncoding());
        System.err.println("	rootGenerator: " + iA.getRootGenerator());
        System.err.println("	generationId: " + iA.getGenerationId());
        System.err.println("	outputPath: " + iA.getOutputPath());
        ExecutorCL executor = new ExecutorCL(iA);

        executor.setup();

        executor.execute();
        System.err.println("... Generation ended");

        return IApplication.EXIT_OK;
    }

    public void stop() {

    }
}
