package io.joynr.generator;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
