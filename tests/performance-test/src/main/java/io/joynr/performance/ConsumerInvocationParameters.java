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

import io.joynr.arbitration.DiscoveryScope;
import io.joynr.messaging.MessagingQosEffort;

/**
 * Takes the command line arguments of the application, parses and checks them.
 * The parsed data is provided through getter methods.
 */
public class ConsumerInvocationParameters {

    /**
     * Determines whether the communication with the provider shall be synchronous or asynchronous.
     */
    enum COMMUNICATIONMODE {
        SYNC, ASYNC
    }

    /**
     * Determines the test case which shall be performed.
     */
    enum TESTCASE {
        SEND_STRING, SEND_STRUCT, SEND_BYTEARRAY, SEND_BYTEARRAY_WITH_SIZE_TIMES_K
    }

    public enum RuntimeConfig {
        IN_PROCESS_CC, WEBSOCKET
    }

    public enum BackendConfig {
        MQTT
    }

    private static final String CMDLINE_OPTIONNAME_DOMAINNAME = "domain";
    private static final String CMDLINE_OPTIONNAME_MESSAGING_QOS_EFFORT = "effort";
    private static final String CMDLINE_OPTIONNAME_NUMRUNS = "runs";
    private static final String CMDLINE_OPTIONNAME_ITERATIONS = "iterations";
    private static final String CMDLINE_OPTIONNAME_PENDING_REQUESTS = "pendingrequests";
    private static final String CMDLINE_OPTIONNAME_NUMTHREADS = "threads";
    private static final String CMDLINE_OPTIONNAME_WARMUPS = "warmups";
    private static final String CMDLINE_OPTIONNAME_SYNCMODE = "syncmode";
    private static final String CMDLINE_OPTIONNAME_TESTCASE = "testcase";
    private static final String CMDLINE_OPTIONNAME_STRINGDATALENGTH = "stringdatalength";
    private static final String CMDLINE_OPTIONNAME_BYTEARRAYSIZE = "bytearraysize";
    private static final String CMDLINE_OPTIONNAME_DISCOVERYSCOPE = "discoveryscope";
    private static final String CMDLINE_OPTIONNAME_RUNTIMECFG = "runtimeconfig";
    private static final String CMDLINE_OPTIONNAME_BACKENDCFG = "backendconfig";
    private static final String CMDLINE_OPTIONNAME_MQTTBROKERURI = "mqttbrokeruri";
    private static final String CMDLINE_OPTIONNAME_CC_HOST = "cchost";
    private static final String CMDLINE_OPTIONNAME_CC_PORT = "ccport";
    private static final String CMDLINE_OPTIONNAME_CONSTANT_NUMBER_OF_PENDING_REQUESTS = "constantnumberofpendingrequests";

    private String domainName = "";
    private MessagingQosEffort effort = MessagingQosEffort.NORMAL;
    private int numberOfRuns = 1;
    private int numberOfIterations = 1;
    private int numberOfpendingRequests = 100;
    private int numberOfThreads = 1;
    private int numberOfWarmupRuns = 0;
    private COMMUNICATIONMODE communicationMode = COMMUNICATIONMODE.SYNC;
    private TESTCASE testCase = TESTCASE.SEND_STRING;
    private int stringDataLength = 10;
    private int byteArraySize = 100;
    private DiscoveryScope discoveryScope = DiscoveryScope.LOCAL_ONLY;
    private RuntimeConfig runtimeConfig = RuntimeConfig.IN_PROCESS_CC;
    private BackendConfig backendConfig = BackendConfig.MQTT;
    private String mqttBrokerUri = "tcp://localhost:1883";
    private String ccHost = "localhost";
    private String ccPort = "4242";
    private boolean constantNumberOfPendingRequests = false;

    public ConsumerInvocationParameters(String[] args) throws Exception {
        CommandLine commandLine = parseCommandLineArgs(args);

        domainName = commandLine.getOptionValue(CMDLINE_OPTIONNAME_DOMAINNAME);

        if (domainName.length() == 0) {
            throw new Exception("Provide a non-empty domain name");
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_MESSAGING_QOS_EFFORT)) {
            effort = MessagingQosEffort.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_MESSAGING_QOS_EFFORT));
        }

        numberOfRuns = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMRUNS)).intValue();

        if (numberOfRuns <= 0) {
            throw new Exception("Number of runs must be positive");
        }

        numberOfWarmupRuns = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_WARMUPS)).intValue();

        if (numberOfWarmupRuns < 0) {
            throw new Exception("Number of warmup runs must be positive or zero");
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_ITERATIONS)) {
            numberOfIterations = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_ITERATIONS)).intValue();

            if (numberOfIterations <= 0) {
                throw new Exception("Number of iterations must be positive");
            }
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_CONSTANT_NUMBER_OF_PENDING_REQUESTS)) {
            constantNumberOfPendingRequests = true;

            if (commandLine.hasOption(CMDLINE_OPTIONNAME_PENDING_REQUESTS)) {
                numberOfpendingRequests = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_PENDING_REQUESTS)).intValue();

                if (numberOfpendingRequests <= 0) {
                    throw new Exception("Number of pending requests must be positive");
                }
            }

            if (commandLine.hasOption(CMDLINE_OPTIONNAME_NUMTHREADS)) {
                numberOfThreads = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMTHREADS)).intValue();

                if (numberOfThreads <= 0) {
                    throw new Exception("Number of threads must be positive");
                }
            }
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_STRINGDATALENGTH)) {
            stringDataLength = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_STRINGDATALENGTH)).intValue();

            if (stringDataLength <= 0) {
                throw new Exception("String data length cannot be less than zero");
            }
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_BYTEARRAYSIZE)) {
            byteArraySize = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_BYTEARRAYSIZE)).intValue();

            if (byteArraySize <= 0) {
                throw new Exception("Byte array size cannot be less than zero");
            }
        }

        // getParsedOptionValue seems not to work for enumerations.
        testCase = TESTCASE.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_TESTCASE));
        if (testCase == TESTCASE.SEND_BYTEARRAY_WITH_SIZE_TIMES_K) {
            testCase = TESTCASE.SEND_BYTEARRAY;
            byteArraySize = byteArraySize * 1000;
        }
        communicationMode = COMMUNICATIONMODE.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_SYNCMODE));

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_DISCOVERYSCOPE)) {
            discoveryScope = DiscoveryScope.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_DISCOVERYSCOPE));
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_BACKENDCFG)) {
            backendConfig = BackendConfig.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_BACKENDCFG));
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_MQTTBROKERURI)) {
            mqttBrokerUri = commandLine.getOptionValue(CMDLINE_OPTIONNAME_MQTTBROKERURI);
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_RUNTIMECFG)) {
            runtimeConfig = RuntimeConfig.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_RUNTIMECFG));
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_CC_HOST)) {
            ccHost = commandLine.getOptionValue(CMDLINE_OPTIONNAME_CC_HOST);
        }

        if (commandLine.hasOption(CMDLINE_OPTIONNAME_CC_PORT)) {
            ccPort = commandLine.getOptionValue(CMDLINE_OPTIONNAME_CC_PORT);
        }
    }

    @SuppressWarnings("checkstyle:methodlength")
    private CommandLine parseCommandLineArgs(String[] args) throws ParseException {
        Options options = new Options();

        options.addOption(Option.builder("s")
                                .longOpt(CMDLINE_OPTIONNAME_SYNCMODE)
                                .required(true)
                                .hasArg()
                                .argName("mode")
                                .type(COMMUNICATIONMODE.class)
                                .desc("Determines how the test methods are called. Can be either SYNC or ASYNC.")
                                .build());

        options.addOption(Option.builder("t")
                                .longOpt(CMDLINE_OPTIONNAME_TESTCASE)
                                .required(true)
                                .hasArg()
                                .argName("testname")
                                .type(TESTCASE.class)
                                .desc("Determines which test is performed. Can be either SEND_STRING, SEND_STRUCT or SEND_BYTEARRAY.")
                                .build());

        options.addOption(Option.builder("d")
                                .longOpt(CMDLINE_OPTIONNAME_DOMAINNAME)
                                .required(true)
                                .hasArg()
                                .argName("domainName")
                                .type(String.class)
                                .desc("Provider domain")
                                .build());

        options.addOption(Option.builder("effort")
                                .longOpt(CMDLINE_OPTIONNAME_MESSAGING_QOS_EFFORT)
                                .required(false)
                                .hasArg()
                                .argName("effort")
                                .type(String.class)
                                .desc("MessagingQosEffort")
                                .build());

        options.addOption(Option.builder("w")
                                .longOpt(CMDLINE_OPTIONNAME_WARMUPS)
                                .required(true)
                                .hasArg()
                                .argName("numWarmups")
                                .type(Number.class)
                                .desc("Number of warmup runs")
                                .build());

        options.addOption(Option.builder("r")
                                .longOpt(CMDLINE_OPTIONNAME_NUMRUNS)
                                .required(true)
                                .hasArg()
                                .argName("numRuns")
                                .type(Number.class)
                                .desc("Number of test runs")
                                .build());

        options.addOption(Option.builder("i")
                                .longOpt(CMDLINE_OPTIONNAME_ITERATIONS)
                                .required(false)
                                .hasArg()
                                .argName("numIterations")
                                .type(Number.class)
                                .desc("Number of test run iterations")
                                .build());

        options.addOption(Option.builder("cnr")
                                .longOpt(CMDLINE_OPTIONNAME_CONSTANT_NUMBER_OF_PENDING_REQUESTS)
                                .required(false)
                                .argName("constantNumberOfPendingRequests")
                                .type(Boolean.class)
                                .desc("TBD")
                                .build());

        options.addOption(Option.builder("pending")
                                .longOpt(CMDLINE_OPTIONNAME_PENDING_REQUESTS)
                                .required(false)
                                .hasArg()
                                .argName("numPendingRequests")
                                .type(Number.class)
                                .desc("Number of pending requests if cnr is enabled")
                                .build());

        options.addOption(Option.builder("threads")
                                .longOpt(CMDLINE_OPTIONNAME_NUMTHREADS)
                                .required(false)
                                .hasArg()
                                .argName("numThreads")
                                .type(Number.class)
                                .desc("Number of threads if cnr is enabled")
                                .build());

        options.addOption(Option.builder("sl")
                                .longOpt(CMDLINE_OPTIONNAME_STRINGDATALENGTH)
                                .required(false)
                                .hasArg()
                                .argName("numChars")
                                .type(Number.class)
                                .desc("Length of strings which are transmitted during the test")
                                .build());

        options.addOption(Option.builder("bs")
                                .longOpt(CMDLINE_OPTIONNAME_BYTEARRAYSIZE)
                                .required(false)
                                .hasArg()
                                .argName("numBytes")
                                .type(Number.class)
                                .desc("Size of byte arrays which are transmitted during the test")
                                .build());

        options.addOption(Option.builder("ds")
                                .longOpt(CMDLINE_OPTIONNAME_DISCOVERYSCOPE)
                                .required(false)
                                .hasArg()
                                .argName("discoveryscope")
                                .type(DiscoveryScope.class)
                                .desc("Determines the discovery scope. Can be GLOBAL_ONLY, LOCAL_AND_GLOBAL, "
                                        + "LOCAL_ONLY or LOCAL_THEN_GLOBAL. Default is LOCAL_ONLY.")
                                .build());

        options.addOption(Option.builder("runtime")
                                .longOpt(CMDLINE_OPTIONNAME_RUNTIMECFG)
                                .required(false)
                                .hasArg()
                                .argName("runtime")
                                .type(RuntimeConfig.class)
                                .desc("Runtime module configuration. " + "Default is " + runtimeConfig.toString())
                                .build());

        options.addOption(Option.builder("backend")
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

        options.addOption(Option.builder("ch")
                                .longOpt(CMDLINE_OPTIONNAME_CC_HOST)
                                .required(false)
                                .hasArg()
                                .argName("cluster-controller host")
                                .type(String.class)
                                .desc("Host of the cluster-controller for websocket connections. Default is " + ccHost)
                                .build());

        options.addOption(Option.builder("cp")
                                .longOpt(CMDLINE_OPTIONNAME_CC_PORT)
                                .required(false)
                                .hasArg()
                                .argName("cluster-controller port")
                                .type(String.class)
                                .desc("Port of the cluster-controller for websocket connections. Default is " + ccPort)
                                .build());

        CommandLineParser parser = new DefaultParser();

        try {
            return parser.parse(options, args);
        } catch (ParseException exception) {
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp("PerformanceTestConsumerApplication", "", options, "", true);
            throw exception;
        }
    }

    public String getDomainName() {
        return domainName;
    }

    public MessagingQosEffort getEffort() {
        return effort;
    }

    public int getNumberOfRuns() {
        return numberOfRuns;
    }

    public int getNumberOfIterations() {
        return numberOfIterations;
    }

    public boolean constantNumberOfPendingRequests() {
        return constantNumberOfPendingRequests;
    }

    public int getNumberOfPendingRequests() {
        return numberOfpendingRequests;
    }

    public int getNumberOfThreads() {
        return numberOfThreads;
    }

    public int getNumberOfWarmupRuns() {
        return numberOfWarmupRuns;
    }

    public COMMUNICATIONMODE getCommunicationMode() {
        return communicationMode;
    }

    public TESTCASE getTestCase() {
        return testCase;
    }

    public int getStringDataLength() {
        return stringDataLength;
    }

    public int getByteArraySize() {
        return byteArraySize;
    }

    public DiscoveryScope getDiscoveryScope() {
        return discoveryScope;
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

    public String getCcHost() {
        return ccHost;
    }

    public String getCcPort() {
        return ccPort;
    }
}
