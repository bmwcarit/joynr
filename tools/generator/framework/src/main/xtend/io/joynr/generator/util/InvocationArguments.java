package io.joynr.generator.util;

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

import io.joynr.generator.IJoynrGenerator;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.reflections.Reflections;

public class InvocationArguments {

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

    private String outputHeaderPath = null;

    private String generationId = null;

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
                setOutputHeaderPath(args[i + 1].replace("\"", ""));
                i++;
            }
        }
        if (!isValid()) {
            System.out.println(getErrorMessage());
        }
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
        if (outputPath == null) {
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

    public String getOutputHeaderPath() {
        if (outputHeaderPath == null) {
            return outputPath + File.separator + "include";
        }
        return outputHeaderPath;
    }

    public void setOutputHeaderPath(String outputHeaderPath) {
        this.outputHeaderPath = outputHeaderPath;
    }

    public String getGenerationId() {
        return generationId;
    }

    public void setGenerationId(String generationId) {
        this.generationId = generationId;
    }

}
