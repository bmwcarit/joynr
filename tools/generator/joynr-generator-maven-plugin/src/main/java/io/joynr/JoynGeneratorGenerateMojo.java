/*
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
package io.joynr;

import java.io.IOException;

import org.apache.maven.plugin.MojoExecutionException;

import io.joynr.generator.GeneratorTask;
import io.joynr.generator.util.InvocationArguments;

/**
 * Goal which generates the joynr interfaces and implementations.
 *
 * @goal generate
 *
 * @phase process-sources
 */
public class JoynGeneratorGenerateMojo extends AbstractJoynGeneratorMojo {
    @Override
    public void execute() throws MojoExecutionException {
        int executionHashCode = createInvocationArguments().getHashCodeForParameterCombination();
        String generationDonePropertyName = "generation.done.id[" + executionHashCode + "]";
        String generationAlreadyDone = project.getProperties().getProperty(generationDonePropertyName);
        if (Boolean.valueOf(generationAlreadyDone)) {
            getLog().info("----------------------------------------------------------------------");
            getLog().info("JOYNR GENERATOR for parameter hash \"" + executionHashCode + "\" already executed.");
            getLog().info("Sources are up-to-date, skipping code generation...");
            getLog().info("----------------------------------------------------------------------");
            return;
        }

        super.execute();
    }

    @Override
    protected void invokeGenerator(GeneratorTask task) throws IOException, ClassNotFoundException,
                                                       InstantiationException, IllegalAccessException {
        task.generate(getLog());
    }

    @Override
    protected InvocationArguments createInvocationArguments() throws MojoExecutionException {
        InvocationArguments result = super.createInvocationArguments();
        result.setGenerate(true);
        result.setClean(false);
        return result;
    }

    @Override
    protected String getSupportedGoal() {
        return "generate";
    }
}
