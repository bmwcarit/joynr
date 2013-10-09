package io.joynr;

/*
 * #%L
 * joynr::tools::generator::Maven joynr Generator Plugin
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

/*
 * Copyright 2001-2005 The Apache Software Foundation.
 *
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
 */

import io.joynr.generator.GeneratorTask;
import io.joynr.generator.util.InvocationArguments;

import org.apache.maven.plugin.AbstractMojo;
import org.apache.maven.plugin.MojoExecutionException;
import org.apache.maven.project.MavenProject;

/**
 * Goal which generates the joynr interfaces and implementations.
 *
 * @goal generate
 * 
 * @phase process-sources
 */
public class JoynGeneratorMojo extends AbstractMojo {
    /**
     * The maven project.
     * 
     * @parameter expression="${project}"
     * @readonly
     */
    private MavenProject project;

    /**
     * The model file.
     * @parameter expression="${joynr.generator.model}"
     * @required
     */
    private String model;

    /**
     * Properties path to the generation templates.
     * @parameter expression="${joynr.generator.templatesDir}"
     */
    private String templatesDir;

    /**
     * Properties encoding of the templates.
     * @parameter expression="${joynr.generator.templatesEncoding}"
     */
    private String templatesEncoding;

    /**
     * Properties full name of the root generator.
     * @parameter expression="${joynr.generator.rootGenerator}"
     */
    private String rootGenerator;

    /**
     * Properties the generation language
     * @parameter expression="${joynr.generator.generationLanguage}"
     */
    private String generationLanguage;

    /**
     * Properties the generationId
     * @parameter expression="${joynr.generator.generationId}"
     */
    private String generationId;

    /**
     * Properties path to the output directory.
     * @parameter expression="${joynr.generator.outputPath}"
     * @required
     */
    private String outputPath;

    /**
     * Properties output directory for header files
     * @parameter expression="${joynr.generator.outputHeaderPath}"
     */
    private String outputHeaderPath;

    /**
     * Properties default resourceEncoding.
     * @parameter expression="${project.build.resourceEncoding}"
     */
    private String defaultEncoding;

    private int getParameterHashCode() {
        StringBuilder sb = new StringBuilder();
        sb.append(model);
        sb.append(templatesDir);
        sb.append(templatesEncoding);
        sb.append(rootGenerator);
        sb.append(generationLanguage);
        sb.append(generationId);
        sb.append(outputPath);
        sb.append(outputHeaderPath);
        return sb.toString().hashCode();
    }

    public void execute() throws MojoExecutionException {
        int executionHashCode = getParameterHashCode();
        String generationDonePropertyName = "generation.done.id[" + executionHashCode + "]";
        String generationAlreadyDone = project.getProperties().getProperty(generationDonePropertyName);
        if (new Boolean(generationAlreadyDone)) {
            getLog().info("----------------------------------------------------------------------");
            getLog().info("JOYNR GENERATOR for parameter hash \"" + executionHashCode + "\" already executed.");
            getLog().info("Sources are up-to-date, skipping code generation...");
            getLog().info("----------------------------------------------------------------------");
            return;
        }

        if (templatesEncoding == null) {
            templatesEncoding = defaultEncoding;
            System.err.println(defaultEncoding);
        }
        getLog().info("----------------------------------------------------------------------");
        getLog().info("JOYNR GENERATOR execution for parameter hash \"" + executionHashCode + "\".");
        getLog().info("----------------------------------------------------------------------");
        getLog().info("model: " + (model == null ? "not specified" : model));
        getLog().info("templatesDir " + (templatesDir == null ? "not specified" : templatesDir));
        getLog().info("templatesEncoding " + (templatesEncoding == null ? "not specified" : templatesEncoding));
        getLog().info("generationLanguage " + (generationLanguage == null ? "not specified" : generationLanguage));
        getLog().info("rootGenerator " + (rootGenerator == null ? "not specified" : rootGenerator));
        getLog().info("generationId " + (generationId == null ? "not specified" : generationId));
        getLog().info("outputPath " + (outputPath == null ? "not specified" : outputPath));
        getLog().info("outputHeaderPath " + (outputHeaderPath == null ? "not specified" : outputHeaderPath));
        getLog().info("----------------------------------------------------------------------");

        try {
            InvocationArguments arguments = new InvocationArguments();
            arguments.setModelpath(model);
            arguments.setTemplatesDir(templatesDir);
            arguments.setTemplatesEncoding(templatesEncoding);
            arguments.setRootGenerator(rootGenerator);
            arguments.setGenerationLanguage(generationLanguage);
            arguments.setGenerationId(generationId);
            arguments.setOutputPath(outputPath);
            arguments.setOutputHeaderPath(outputHeaderPath);

            GeneratorTask task = new GeneratorTask(arguments);
            task.generate();
        } catch (Exception e) {
            getLog().info(e);
            throw new MojoExecutionException("Failed to execute JOYNR Generator", e);
        }
        project.getProperties().put(generationDonePropertyName, "true");
    }
}
