/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

package io.joynr.performance;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import joynr.types.ProviderScope;

public class EchoProviderInvocationParameters {

    public enum RuntimeConfig {
        IN_PROCESS_CC
    }

    public enum BackendConfig {
        MQTT
    }

    private static final String CMDLINE_OPTIONNAME_DOMAINNAME = "domain";
    private static final String CMDLINE_OPTIONNAME_SCOPE = "scope";
    private static final String CMDLINE_OPTIONNAME_RUNTIMECFG = "runtimeconfig";
    private static final String CMDLINE_OPTIONNAME_BACKENDCFG = "backendconfig";
    private static final String CMDLINE_OPTIONNAME_MQTTBROKERURI = "mqttbrokeruri";

    private String domain = "";
    private ProviderScope providerScope = ProviderScope.GLOBAL;

    private RuntimeConfig runtimeConfig = RuntimeConfig.IN_PROCESS_CC;
    private BackendConfig backendConfig = BackendConfig.MQTT;

    private String mqttBrokerUri = "tcp://localhost:1883";

    public EchoProviderInvocationParameters(String[] args) throws Exception {
        CommandLine cmdLine = parseCommandLineArgs(args);

        domain = cmdLine.getOptionValue(CMDLINE_OPTIONNAME_DOMAINNAME);

        if (cmdLine.hasOption(CMDLINE_OPTIONNAME_SCOPE)) {
            providerScope = ProviderScope.valueOf(cmdLine.getOptionValue(CMDLINE_OPTIONNAME_SCOPE));
        }

        if (cmdLine.hasOption(CMDLINE_OPTIONNAME_RUNTIMECFG)) {
            runtimeConfig = RuntimeConfig.valueOf(cmdLine.getOptionValue(CMDLINE_OPTIONNAME_RUNTIMECFG));
        }

        if (cmdLine.hasOption(CMDLINE_OPTIONNAME_BACKENDCFG)) {
            backendConfig = BackendConfig.valueOf(cmdLine.getOptionValue(CMDLINE_OPTIONNAME_BACKENDCFG));
        }

        if (cmdLine.hasOption(CMDLINE_OPTIONNAME_MQTTBROKERURI)) {
            mqttBrokerUri = cmdLine.getOptionValue(CMDLINE_OPTIONNAME_MQTTBROKERURI);
        }
    }

    private CommandLine parseCommandLineArgs(String[] args) throws ParseException {
        Options options = new Options();

        options.addOption(Option.builder("d")
                                .longOpt(CMDLINE_OPTIONNAME_DOMAINNAME)
                                .required(true)
                                .hasArg()
                                .argName("domain")
                                .type(String.class)
                                .desc("Provider domain")
                                .build());

        options.addOption(Option.builder("s")
                                .longOpt(CMDLINE_OPTIONNAME_SCOPE)
                                .required(false)
                                .hasArg()
                                .argName("scope")
                                .type(ProviderScope.class)
                                .desc("Scope of the provider. Can be LOCAL or GLOBAL. Default is "
                                        + providerScope.toString())
                                .build());

        options.addOption(Option.builder("r")
                                .longOpt(CMDLINE_OPTIONNAME_RUNTIMECFG)
                                .required(false)
                                .hasArg()
                                .argName("runtime")
                                .type(RuntimeConfig.class)
                                .desc("Runtime module configuration. "
                                        + "At the moment only IN_PROCESS_CC is supported. Default is "
                                        + runtimeConfig.toString())
                                .build());

        options.addOption(Option.builder("b")
                                .longOpt(CMDLINE_OPTIONNAME_BACKENDCFG)
                                .required(false)
                                .hasArg()
                                .argName("backend")
                                .type(BackendConfig.class)
                                .desc("Backend configuration. At the moment only MQTT is supported. Default is "
                                        + backendConfig.toString())
                                .build());

        options.addOption(Option.builder("mbu")
                                .longOpt(CMDLINE_OPTIONNAME_MQTTBROKERURI)
                                .required(false)
                                .hasArg()
                                .argName("uri")
                                .type(String.class)
                                .desc("MQTT broker URI. Default is " + mqttBrokerUri)
                                .build());

        CommandLineParser parser = new DefaultParser();

        try {
            return parser.parse(options, args);
        } catch (ParseException exception) {
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp("PerformanceTestEchoProviderApplication", "", options, "", true);
            throw exception;
        }
    }

    public ProviderScope getProviderScope() {
        return providerScope;
    }

    public String getDomain() {
        return domain;
    }

    public String getMqttBrokerUri() {
        return mqttBrokerUri;
    }

    public RuntimeConfig getRuntimeMode() {
        return runtimeConfig;
    }

    public BackendConfig getBackendTransportMode() {
        return backendConfig;
    }
}
