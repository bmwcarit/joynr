/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

package io.joynr.performancemeasurement;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.DefaultParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

public class ApplicationInvocationParameters {
    enum TESTCASE {
        REQUESTS_ONLY, REQUESTS_WITH_PROXY
    }

    private static final String CMDLINE_OPTIONNAME_TESTCASE = "testcase";
    private static final String CMDLINE_OPTIONNAME_NUMBER_OF_REQUEST_CALLS = "reqcalls";
    private static final String CMDLINE_OPTIONNAME_NUMBER_OF_MAXINFLIGHT_CALLS = "maxinflight";
    private static final String CMDLINE_OPTIONNAME_NUMBER_OF_PROXY_CREATIONS = "proxycreations";
    private static final String CMDLINE_OPTIONNAME_FILENAME = "filename";
    private static final String CMDLINE_OPTIONNAME_NUMBER_OF_ITERATIONS = "iterations";

    private TESTCASE testCase = TESTCASE.REQUESTS_ONLY;
    private int numberOfRequestCalls = 10000;
    private int maxInflightNumberOfCalls = 100;
    private int numberOfProxyCreations = 2000;
    private String fileName = "PerformanceMeasurementTest.csv";
    private int numOfIterations = 1;

    public ApplicationInvocationParameters(String[] args) throws Exception {
        CommandLine commandLine = parseCommandLineArgs(args);

        testCase = TESTCASE.valueOf(commandLine.getOptionValue(CMDLINE_OPTIONNAME_TESTCASE));
        fileName = commandLine.getOptionValue(CMDLINE_OPTIONNAME_FILENAME);

        numberOfRequestCalls = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMBER_OF_REQUEST_CALLS)).intValue();
        if (numberOfRequestCalls <= 0) {
            throw new Exception("Number of request calls must be positive");
        }

        numberOfProxyCreations = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMBER_OF_PROXY_CREATIONS)).intValue();
        if (numberOfProxyCreations <= 0) {
            throw new Exception("Number of proxy creation calls must be positive");
        }

        maxInflightNumberOfCalls = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMBER_OF_MAXINFLIGHT_CALLS)).intValue();
        if (maxInflightNumberOfCalls <= 0) {
            throw new Exception("Number of max inflight number of performing requests must be positive");
        }

        numOfIterations = ((Number) commandLine.getParsedOptionValue(CMDLINE_OPTIONNAME_NUMBER_OF_ITERATIONS)).intValue();
        if (numOfIterations <= 0) {
            throw new Exception("Number of iterations must be positive");
        }
    }

    private CommandLine parseCommandLineArgs(String[] args) throws ParseException {
        Options options = new Options();

        options.addOption(Option.builder("t")
                                .longOpt(CMDLINE_OPTIONNAME_TESTCASE)
                                .required(true)
                                .hasArg()
                                .argName("testname")
                                .type(TESTCASE.class)
                                .desc("Determines which test is performed. Can be either REQUESTS_ONLY or REUQESTS_WITH_PROXY.")
                                .build());

        options.addOption(Option.builder("r")
                                .longOpt(CMDLINE_OPTIONNAME_NUMBER_OF_REQUEST_CALLS)
                                .required(true)
                                .hasArg()
                                .argName("numOfRequestCalls")
                                .type(Number.class)
                                .desc("Number of requests to perform asynchronously.")
                                .build());

        options.addOption(Option.builder("m")
                                .longOpt(CMDLINE_OPTIONNAME_NUMBER_OF_MAXINFLIGHT_CALLS)
                                .required(true)
                                .hasArg()
                                .argName("maxInflightCalls")
                                .type(Number.class)
                                .desc("Max number of requests performning asynchronously inflight.")
                                .build());

        options.addOption(Option.builder("p")
                                .longOpt(CMDLINE_OPTIONNAME_NUMBER_OF_PROXY_CREATIONS)
                                .required(true)
                                .hasArg()
                                .argName("numOfProxyCalls")
                                .type(Number.class)
                                .desc("Number of proxy creation operations to perform asynchronously in a separate thread along with a request performance.")
                                .build());

        options.addOption(Option.builder("f")
                                .longOpt(CMDLINE_OPTIONNAME_FILENAME)
                                .required(true)
                                .hasArg()
                                .argName("filename")
                                .type(String.class)
                                .desc("Name of csv-file to save collected test data.")
                                .build());

        options.addOption(Option.builder("i")
                                .longOpt(CMDLINE_OPTIONNAME_NUMBER_OF_ITERATIONS)
                                .required(true)
                                .hasArg()
                                .argName("numOfIterations")
                                .type(Number.class)
                                .desc("Number of test performance iterations.")
                                .build());

        CommandLineParser parser = new DefaultParser();

        try {
            return parser.parse(options, args);
        } catch (ParseException exception) {
            HelpFormatter formatter = new HelpFormatter();
            formatter.printHelp("PerformanceMeasurement:", "", options, "", true);
            throw exception;
        }
    }

    public TESTCASE getTestCase() {
        return testCase;
    }

    public String getFileName() {
        return fileName;
    }

    public int getNumberOfRequestCalls() {
        return numberOfRequestCalls;
    }

    public int getNumberOfProxyCreations() {
        return numberOfProxyCreations;
    }

    public int getNumberOfMaxInflightCalls() {
        return maxInflightNumberOfCalls;
    }

    public int getNumberOfIterations() {
        return numOfIterations;
    }
}
