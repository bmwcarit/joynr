package io.joynr.generator.util;

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

import io.joynr.generator.IJoynrGenerator;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.reflections.Reflections;

public class InvocationArguments {

    public static final String OUTPUT_PATH = "outputPath";

    // A lookup of language to root generator
    protected static Map<String, String> languages = new HashMap<String, String>();
    static {
        Reflections reflections = new Reflections("io", "com", "de", "org");
        Set<Class<? extends IJoynrGenerator>> generators = reflections.getSubTypesOf(IJoynrGenerator.class);
        for (Class<? extends IJoynrGenerator> generator : generators) {
            try {
                IJoynrGenerator instance = generator.newInstance();
                languages.put(instance.getLanguageId(), generator.getName());
            } catch (Exception e) {
            }
        }
    }

    private String modelpath = null;

    private String templatesDir = null;

    private String rootGenerator = null;

    private String templatesEncoding = null;

    private String outputPath = null;

    private String generationId = null;

    private Map<String, String> parameter;

    private boolean generate = true;

    public InvocationArguments() {
    }

    public InvocationArguments(String[] args) throws IllegalStateException {
        parseArguments(args);

        if (!new File(modelpath).exists()) {
            throw new IllegalStateException("Path to model \"" + modelpath + "\" is not correct!"
                    + dumpCorrectInvocation());
        }
        if (rootGenerator == null) {
            throw new IllegalStateException("Root generator could not be found!" + dumpCorrectInvocation());
        }
    }

    private String getLanguages() {
        StringBuffer appender = new StringBuffer();
        for (String language : languages.keySet()) {
            appender.append(language + ", ");
        }
        if (!languages.isEmpty()) {
            appender.delete(appender.length() - 2, appender.length());
        } else {
            appender.append("none");
        }
        return appender.toString();
    }

    private String dumpCorrectInvocation() {
        String result = "\n";
        result += ("------------------------------------------------------------------------\n");
        result += ("Generator could not be started due to wrong parameter settings!\n");
        result += ("------------------------------------------------------------------------\n");
        result += ("Start the application with the following params: \n");
        result += ("      Required: \n");
        result += ("       -modelpath <path to model>\n");
        result += ("       -outputPath <path to output directory>\n");
        result += ("      Also one of:\n");
        result += ("       -rootGenerator <full name of template root> OR\n");
        result += ("       -generationLanguage <cpp|java|javascript>\n");
        result += ("      Optional: \n");
        result += ("       -templatesDir <folder name of templates directory>\n");
        result += ("       -templatesEncoding <encoding of templates>\n");
        result += ("       -generationId <name of what is being generated>\n");
        result += ("      Optional, C++ only: \n");
        result += ("       -outputHeaderPath <path to directory containing header files>\n");
        result += ("       -includePrefix <prefix to use in include statements>\n");
        return result;
    }

    public void parseArguments(String[] args) {
        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-templatesDir")) {
                setTemplatesDir(new File(args[i + 1]).getAbsolutePath());
                i++;
            } else if (args[i].equals("-modelpath")) {
                setModelpath(args[i + 1]);
                i++;
            } else if (args[i].equals("-rootGenerator")) {
                setRootGenerator(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equals("-generationLanguage")) {
                setGenerationLanguage(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equals("-outputPath")) {
                setOutputPath(new File(args[i + 1]).getAbsolutePath());
                i++;
            } else if (args[i].equals("-templatesEncoding")) {
                setTemplatesEncoding(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equals("-generationId")) {
                setGenerationId(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equals("-outputHeaderPath")) {
                setParameterElement("outputHeaderPath", args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equals("-clean")) {
                setGenerate(false);
            }
        }
        if (!isValid()) {
            System.out.println(getErrorMessage());
        }
    }

    public void setGenerate(boolean generate) {
        this.generate = generate;
    }

    private void setParameterElement(String key, String value) {
        if (parameter == null) {
            parameter = new HashMap<String, String>();
        }

        parameter.put(key, value);

    }

    public boolean shallGenerate() {
        return generate;
    }

    public boolean isValid() {
        return (outputPath != null && modelpath != null && rootGenerator != null);
    }

    // Return an error message describing what is invalid
    public String getErrorMessage() {
        StringBuffer message = new StringBuffer();
        message.append("Please set:\n");
        if (outputPath == null) {
            message.append("outputPath\n");
        }
        if (modelpath == null) {
            message.append("modelpath\n");
        }
        if (rootGenerator == null) {
            message.append("rootGenerator or generationLanguage\n");
        }
        return message.toString();
    }

    public String getModelpath() {
        return modelpath;
    }

    public void setModelpath(String modelpath) {
        this.modelpath = modelpath;
    }

    public String getTemplatesDir() {
        return templatesDir;
    }

    public void setTemplatesDir(String templatesDir) {
        this.templatesDir = templatesDir;
    }

    public String getRootGenerator() {
        return rootGenerator;
    }

    public void setRootGenerator(String rootGenerator) {
        this.rootGenerator = rootGenerator;
    }

    public void setGenerationLanguage(String generationLanguage) {
        if (rootGenerator == null && generationLanguage != null) {
            this.rootGenerator = languages.get(generationLanguage);
            if (rootGenerator == null) {
                throw new IllegalStateException("The generation language \""
                        + generationLanguage
                        + "\" could not be found in the configuration. The following languages have been found: "
                        + getLanguages()
                        + ". Be sure to have the respective generation templates included in your dependencies. The package of generator templates shall start with \"io\", \"com\", \"org\" or \"de\"");
            }
        }
    }

    public String getTemplatesEncoding() {
        return templatesEncoding;
    }

    public void setTemplatesEncoding(String templatesEncoding) {
        this.templatesEncoding = templatesEncoding;
    }

    public String getOutputPath() {
        return outputPath;
    }

    public void setOutputPath(String outputPath) {
        this.outputPath = outputPath;
    }

    public Map<String, String> getParameter() {
        if (parameter == null) {
            parameter = new HashMap<String, String>();
        }
        if (!parameter.containsKey(OUTPUT_PATH)) {
            parameter.put(OUTPUT_PATH, getOutputPath());
        }
        return parameter;
    }

    public void setParameter(Map<String, String> parameter) {
        this.parameter = parameter;
    }

    public String getGenerationId() {
        return generationId;
    }

    public void setGenerationId(String generationId) {
        this.generationId = generationId;
    }

}
