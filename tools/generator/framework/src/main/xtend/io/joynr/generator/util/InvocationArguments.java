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
package io.joynr.generator.util;

import java.io.File;
import java.util.HashMap;
import java.util.Map;
import java.util.Set;

import org.apache.log4j.Logger;
import org.reflections.Reflections;

import io.joynr.generator.IJoynrGenerator;

public class InvocationArguments {
    private static Logger logger = Logger.getLogger(InvocationArguments.class);

    public static final String OUTPUT_PATH = "outputPath";

    // A lookup of language to root generator
    protected static Map<String, String> languages = new HashMap<>();
    static {
        Reflections reflections = new Reflections("io", "com", "de", "org");
        Set<Class<? extends IJoynrGenerator>> generators = reflections.getSubTypesOf(IJoynrGenerator.class);
        for (Class<? extends IJoynrGenerator> generator : generators) {
            try {
                IJoynrGenerator instance = generator.newInstance();
                languages.put(instance.getLanguageId(), generator.getName());
            } catch (Exception e) {
                logger.error("Unable to load language generator:" + generator.getName(), e);
            }
        }
    }

    private String modelPath = null;

    private String rootGenerator = null;

    private String outputPath = null;

    private String generationId = null;

    private Map<String, String> parameter;

    private boolean generate = true;

    private boolean clean = false;

    private String addVersionTo = "none";

    private boolean generateProxyCode = true;

    private boolean generateProviderCode = true;

    public InvocationArguments() {
        // allows setting args programmatically
    }

    public InvocationArguments(String[] args) {
        if (args.length == 0) {
            throw new IllegalArgumentException("No parameters provided!" + dumpCorrectInvocation());
        }
        parseArguments(args);
        checkArguments(true);
    }

    private static String getLanguages(String seperator) {
        assert seperator != null;
        StringBuilder appender = new StringBuilder();
        for (String language : languages.keySet()) {
            appender.append(language + seperator);
        }
        if (!languages.isEmpty()) {
            appender.delete(appender.length() - seperator.length(), appender.length());
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
        result += usageString();
        return result;
    }

    public static String usageString() {
        StringBuilder usageString = new StringBuilder();
        usageString.append("Start the application with the following parameters: \n");
        usageString.append("      Required: \n");
        usageString.append("       " + dumpModelPathDefinition() + "\n");
        usageString.append("       " + dumpOutputPathDefinition() + "\n");
        usageString.append("      One of:\n");
        usageString.append("       " + dumpRootGeneratorDefinition() + " OR\n");
        usageString.append("       " + dumpGenerationLanguageDefinition() + "\n");
        usageString.append("      Optional: \n");
        usageString.append("       -templatesDir <folder name of templates directory>\n");
        usageString.append("       -templatesEncoding <encoding of templates>\n");
        usageString.append("       -generationId <name of what is being generated>\n");
        usageString.append("       " + dumpVersionDefinition() + "\n");
        usageString.append("         DEPRECATED! Set the #noVersionGeneration comment in the .fidl file instead. See generator documentation for more information.\n");
        usageString.append("         If this option is 'package' when #noVersionGeneration is set, this leads to an Exception, and the generation will be aborted.\n");
        usageString.append("         If #noVersionGeneration is not set, this option has to be absent or 'package', else an Exception as well as an abort will occur as well.\n");
        usageString.append("         Generally, it's best not to use this setting at all since it will be removed very soon.\n");
        usageString.append("         package: interface/typecollection major versions (if existing) are added as an additional package \"v<version>\"\n");
        usageString.append("         none: interface/typecollection versions do not affect the generated name and package of interfaces and types\n");
        usageString.append("         default: none\n");
        usageString.append("         Note:\n");
        usageString.append("           - Consumer and provider applications of one interface have to use the same versioning scheme to be able to communicate with each other.\n");
        usageString.append("           - The feature has been fully tested to work in Java, in C++ and JS only the versioning of interfaces has been tested so far but the versioning of types is expected to work as well.\n");
        usageString.append("      Optional, C++ only: \n");
        usageString.append("       -outputHeaderPath <path to directory containing header files>\n");
        usageString.append("       -includePrefix <prefix to use in include statements>\n");
        usageString.append("      Optional:\n");
        usageString.append("       -target proxy|provider|both:\n");
        return usageString.toString();
    }

    private static String dumpOutputPathDefinition() {
        return "-outputPath <path to output directory>";
    }

    private static String dumpModelPathDefinition() {
        return "-modelPath <path to model>";
    }

    private static String dumpRootGeneratorDefinition() {
        return "-rootGenerator <full name of template root>";
    }

    private static String dumpVersionDefinition() {
        return "-addVersionTo <package, none>";
    }

    private static String dumpGenerationLanguageDefinition() {
        return "-generationLanguage <" + getLanguages("|") + ">";
    }

    public void parseArguments(String[] args) {
        for (int i = 0; i < args.length; i++) {
            if (args[i].equalsIgnoreCase("-clean")) {
                setClean(true);
            } else if (args[i].equalsIgnoreCase("-modelPath")) {
                setModelPath(args[i + 1]);
                i++;
            } else if (args[i].equalsIgnoreCase("-generate")) {
                setGenerate(args[i + 1].equalsIgnoreCase("true"));
                i++;
            } else if (args[i].equalsIgnoreCase("-addVersionTo")) {
                logger.warn("DEPRECATION WARNING: Usage of outdated -addVersionTo option detected."
                        + " Set the #noVersionGeneration comment in fidl files instead.");
                setAddVersionTo(args[i + 1]);
                i++;
            } else if (args[i].equalsIgnoreCase("-generationId")) {
                setGenerationId(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-generationLanguage")) {
                setGenerationLanguage(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-jee")) {
                setParameterElement("jee", args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-ignoreInvalidNullClassMembers")) {
                setParameterElement("ignoreInvalidNullClassMembers", args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-outputHeaderPath")) {
                setParameterElement("outputHeaderPath", args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-outputPath")) {
                setOutputPath(new File(args[i + 1]).getAbsolutePath());
                i++;
            } else if (args[i].equalsIgnoreCase("-rootGenerator")) {
                setRootGenerator(args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-requireJSSupport")) {
                setParameterElement("requireJSSupport", args[i + 1].replace("\"", ""));
                i++;
            } else if (args[i].equalsIgnoreCase("-target")) {
                setTarget(args[i + 1]);
                i++;
            } else if (args[i].equalsIgnoreCase("-requireJSSupport")) {
                setParameterElement("requireJSSupport", args[i + 1].replace("\"", ""));
                i++;
            }
        }
    }

    public void setTarget(String target) {
        if (target == null) {
            throw new IllegalArgumentException("-target called with illegal parameter null");
        } else if (target.equals("proxy")) {
            setGenerateProviderCode(false);
        } else if (target.equals("provider")) {
            setGenerateProxyCode(false);
        } else if (target.equals("both")) {
            // no action required, default is already true
        } else {
            throw new IllegalArgumentException("-target called with illegal parameter " + target);
        }
    }

    public void setClean(boolean clean) {
        this.clean = clean;
    }

    public void setGenerate(boolean generate) {
        this.generate = generate;
    }

    public void setAddVersionTo(String addVersionTo) {
        this.addVersionTo = addVersionTo;
    }

    private void setParameterElement(String key, String value) {
        if (parameter == null) {
            parameter = new HashMap<>();
        }

        parameter.put(key, value);

    }

    public boolean generate() {
        return generate;
    }

    public boolean clean() {
        return clean;
    }

    public boolean addVersionToPackage() {
        return "package".equalsIgnoreCase(addVersionTo);
    }

    public void checkArguments() {
        checkArguments(false);
    }

    public void checkArguments(boolean checkIfModelPathIsValid) {
        StringBuilder errorMessages = new StringBuilder();
        String newLine = System.getProperty("line.separator");
        if (outputPath == null) {
            errorMessages.append("- Output path is missing. Please invoke the generator with the following argument: "
                    + dumpOutputPathDefinition());
            errorMessages.append(newLine);
        }
        if (modelPath == null) {
            errorMessages.append("- Model path is missing. Please invoke the generator with the following argument: "
                    + dumpModelPathDefinition());
            errorMessages.append(newLine);
        } else if (checkIfModelPathIsValid && !new File(modelPath).exists()) {
            errorMessages.append("- Path to model \"" + modelPath + "\" is not correct. File could not be found");
            errorMessages.append(newLine);
        }
        if (rootGenerator == null) {
            errorMessages.append("- Root generator could not be found. Please invoke the generator with the following argument: "
                    + dumpRootGeneratorDefinition() + " OR " + dumpGenerationLanguageDefinition());
            errorMessages.append(newLine);
        }
        if (addVersionTo != null
                && !(addVersionTo.equalsIgnoreCase("package") || addVersionTo.equalsIgnoreCase("none"))) {
            errorMessages.append("- Version inclusion specifier was invalid. Please invoke the generator with the following argument: "
                    + dumpVersionDefinition());
            errorMessages.append(newLine);
        }

        if (errorMessages.length() > 0) {
            throw new IllegalArgumentException("Invocation arguments are not set properly. See the following error messages for further details "
                    + newLine + errorMessages.toString() + dumpCorrectInvocation());
        }
    }

    public String getModelPath() {
        return modelPath;
    }

    public void setModelPath(String modelPath) {
        this.modelPath = modelPath;
    }

    public String getRootGenerator() {
        return rootGenerator;
    }

    public void setRootGenerator(String rootGenerator) {
        this.rootGenerator = rootGenerator;
    }

    public void setGenerationLanguage(String generationLanguage) {
        if (rootGenerator == null && generationLanguage != null) {
            rootGenerator = languages.get(generationLanguage);
            if (rootGenerator == null) {
                throw new IllegalArgumentException("The generation language \"" + generationLanguage
                        + "\" could not be found in the configuration. The following languages have been found: "
                        + getLanguages(", ")
                        + ". Be sure to have the respective generation templates included in your dependencies. The package of generator templates shall start with \"io\", \"com\", \"org\" or \"de\""
                        + dumpCorrectInvocation());
            }
        }
    }

    public String getOutputPath() {
        return outputPath;
    }

    public void setOutputPath(String outputPath) {
        this.outputPath = outputPath;
    }

    public Map<String, String> getParameter() {
        if (parameter == null) {
            parameter = new HashMap<>();
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

    public int getHashCodeForParameterCombination() {
        StringBuilder sb = new StringBuilder();
        sb.append(getModelPath());
        sb.append(getRootGenerator());
        sb.append(generationId);
        sb.append(addVersionTo);
        sb.append(outputPath);
        if (parameter != null) {
            for (Map.Entry<String, String> entry : parameter.entrySet()) {
                sb.append(entry.getKey());
                sb.append(entry.getValue());
            }
        }
        sb.append(generate());
        sb.append(clean());
        return sb.toString().hashCode();
    }

    public void setGenerateProxyCode(boolean generateProxyCode) {
        this.generateProxyCode = generateProxyCode;
    }

    public boolean getGenerateProxyCode() {
        return generateProxyCode;
    }

    public void setGenerateProviderCode(boolean generateProviderCode) {
        this.generateProviderCode = generateProviderCode;
    }

    public boolean getGenerateProviderCode() {
        return generateProviderCode;
    }

}
