/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
package joynr.tests.performance;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

/**
 * Takes the command line arguments of the application, parses and checks them.
 * The parsed data is provided through getter methods.
 */
public class PerformanceTestConsumerInvocationParameters {

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
        SEND_STRING, SEND_STRUCT, SEND_BYTEARRAY
    }

    private static final String CMDLINE_OPTIONNAME_DOMAINNAME = "domain";
    private static final String CMDLINE_OPTIONNAME_NUMRUNS = "runs";
    private static final String CMDLINE_OPTIONNAME_WARMUPS = "warmups";
    private static final String CMDLINE_OPTIONNAME_SYNCMODE = "syncmode";
    private static final String CMDLINE_OPTIONNAME_TESTCASE = "testcase";
    private static final String CMDLINE_OPTIONNAME_STRINGDATALENGTH = "stringdatalength";
    private static final String CMDLINE_OPTIONNAME_BYTEARRAYSIZE = "bytearraysize";

    private static String domainName = "";
    private static int numberOfRuns = 1;
    private static int numberOfWarmupRuns = 0;
    private static COMMUNICATIONMODE communicationMode = COMMUNICATIONMODE.SYNC;
    private static TESTCASE testCase = TESTCASE.SEND_STRING;
    private static int stringDataLength = 10;
    private static int byteArraySize = 100;

    public PerformanceTestConsumerInvocationParameters(String[] args) throws Exception {
        CommandLine commandLine = parseCommandLineArgs(args);

        domainName = commandLine.getOptionValue(CMDLINE_OPTIONNAME_DOMAINNAME);

        if (domainName.length() == 0) {
            throw new Exception("Provide a non-empty domain name");
        }

        numberOfRuns = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMRUNS)).intValue();

        if (numberOfRuns <= 0) {
            throw new Exception("Number of runs must be positive");
        }

        numberOfWarmupRuns = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_WARMUPS)).intValue();

        if (numberOfWarmupRuns < 0) {
            throw new Exception("Number of warmup runs must be positive or zero");
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
        communicationMode = COMMUNICATIONMODE.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_SYNCMODE));
    }

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

    public int getNumberOfRuns() {
        return numberOfRuns;
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
}
