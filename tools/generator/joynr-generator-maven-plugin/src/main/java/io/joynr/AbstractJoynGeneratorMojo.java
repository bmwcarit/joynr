/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2020 BMW Car IT GmbH
 * Copyright 2001-2005 The Apache Software Foundation.
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
import java.util.HashMap;
import java.util.Map;

import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.project.MavenProject;

import io.joynr.generator.GeneratorTask;
import io.joynr.generator.util.InvocationArguments;

/**
 * Abstract class for the joynr generator mojos
 *
 */
public abstract class AbstractJoynGeneratorMojo extends AbstractMojo {
    /**
     * The maven project.
     * 
     * @parameter property="project"
     * @readonly
     */
    protected MavenProject project;

    /**
     * The model file.
     * @parameter property="joynr.generator.model"
     * @required
     */
    protected String model;

    /**
     * Properties full name of the root generator.
     * @parameter property="joynr.generator.rootGenerator"
     */
    protected String rootGenerator;

    /**
     * Properties the generation language
     * @parameter property="joynr.generator.generationLanguage"
     */
    protected String generationLanguage;

    /**
     * Properties the generationId
     * @parameter property="joynr.generator.generationId"
     */
    protected String generationId;

    /**
     * Properties where the version shall be included in (package, name, none)
     * @parameter property="joynr.generator.addVersionTo"
     */
    protected String addVersionTo;

    /**
     * Properties path to the output directory.
     * @parameter property="joynr.generator.outputPath"
     * @required
     */
    protected String outputPath;

    /**
     * Properties map of additional parameters required by the custom generators
     * @parameter property="joynr.generator.parameter"
     */
    protected Map<String, String> parameter;

    /**
     * Properties skips the generation, if set to true
     * @parameter property="joynr.generator.skip"
     */
    protected String skip;

    public void execute() throws MojoExecutionException {
        InvocationArguments arguments = createInvocationArguments();
        int executionHashCode = arguments.getHashCodeForParameterCombination();
        String generationDonePropertyName = "generation.done.id[" + executionHashCode + "]";

        getLog().info("----------------------------------------------------------------------");
        getLog().info("JOYNR GENERATOR execution for parameter hash \"" + executionHashCode + "\".");
        getLog().info("----------------------------------------------------------------------");
        if (skip != null && skip.equalsIgnoreCase("true")) {
            getLog().info("skip plugin execution due to maven plugin configuration");
            return;
        }
        getLog().info("model: " + (model == null ? "not specified" : model));
        getLog().info("generationLanguage " + (generationLanguage == null ? "not specified" : generationLanguage));
        getLog().info("rootGenerator " + (rootGenerator == null ? "not specified" : rootGenerator));
        getLog().info("generationId " + (generationId == null ? "not specified" : generationId));
        getLog().info("outputPath " + (outputPath == null ? "not specified" : outputPath));
        getLog().info("addVersionTo " + (addVersionTo == null ? "not specified" : addVersionTo));
        getLog().info("parameter " + (parameter == null ? "not specified" : ":"));
        if (parameter != null) {
            for (Map.Entry<String, String> entry : parameter.entrySet()) {
                getLog().info("   " + entry.getKey() + ": " + entry.getValue());
            }
        }
        getLog().info("----------------------------------------------------------------------");

        try {
            GeneratorTask task = new GeneratorTask(arguments);
            invokeGenerator(task);
            project.getProperties().put(generationDonePropertyName, "true");
        } catch (Exception e) {
            getLog().info(e);
            throw new MojoExecutionException("Failed to execute joynr generator plugin", e);
        }
    }

    protected abstract void invokeGenerator(GeneratorTask task) throws IOException, ClassNotFoundException,
                                                                InstantiationException, IllegalAccessException;

    protected InvocationArguments createInvocationArguments() {
        InvocationArguments arguments = new InvocationArguments();
        arguments.setModelPath(model);
        arguments.setRootGenerator(rootGenerator);
        arguments.setGenerationLanguage(generationLanguage);
        arguments.setGenerationId(generationId);
        arguments.setOutputPath(outputPath);
        arguments.setAddVersionTo(addVersionTo);
        arguments.setParameter(parameter);
        if (parameter == null) {
            parameter = new HashMap<String, String>();
        }
        return arguments;
    }

    protected abstract String getSupportedGoal();
}
