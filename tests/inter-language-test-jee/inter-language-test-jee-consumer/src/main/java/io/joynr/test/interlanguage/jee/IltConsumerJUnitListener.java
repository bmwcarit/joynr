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
package io.joynr.test.interlanguage.jee;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.junit.runner.Description;
import org.junit.runner.Result;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.ilt.testresults.TestCaseFailure;
import io.joynr.ilt.testresults.TestCaseResult;
import io.joynr.ilt.testresults.TestResult;
import io.joynr.ilt.testresults.TestSuiteResult;

public class IltConsumerJUnitListener extends RunListener {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerJUnitListener.class);
    private TestResult testResult = null;
    ArrayList<TestCaseResult> testCaseResults = new ArrayList<TestCaseResult>();

    static class TestSuiteResultsStore {
        public ArrayList<TestCaseResult> testCaseResults = new ArrayList<TestCaseResult>();
        public int errors = 0;
        public int tests = 0;
        public int skipped = 0;
        public int failures = 0;
        public long consumedTime = 0;
    }

    // Map testSuiteName -> testSuiteResultsStore
    HashMap<String, TestSuiteResultsStore> testSuitesMap = new HashMap<String, TestSuiteResultsStore>();

    long startTimeTestCase;

    // for manual invocation after the test is finished
    public TestResult getTestResult() {
        logger.info(">>> getTestResult called");
        logger.info("<<< getTestResult called");
        return testResult;
    }

    // helper methods
    public void printDescription(Description description, int level) {
        String spaces = new String(new char[level * 2]).replace('\0', ' ');
        if (description == null) {
            logger.info(spaces + "description is null");
        } else {
            logger.info(spaces + "description is set");

            logger.info(spaces + "- description.getDisplayName() = " + description.toString());
            logger.info(spaces + "- description.isEmpty() = " + description.isEmpty());
            logger.info(spaces + "- description.isSuite() = " + description.isSuite());
            logger.info(spaces + "- description.isTest() = " + description.isTest());
            logger.info(spaces + "- description.testCount() = " + description.testCount());
            ArrayList<Description> children = description.getChildren();
            for (int i = 0; i < children.size(); i++) {
                printDescription(children.get(i), level + 1);
            }

            logger.info(spaces + "- description.toString() = " + description.toString());
        }
    }

    public String getTestSuiteClassName(Description description) {
        String fullTestClassName;
        Pattern pattern = Pattern.compile("\\((.*)\\)");
        Matcher matcher = pattern.matcher(description.getDisplayName());
        if (matcher.find()) {
            //fullTestClassName = this.getClass().getPackage() + matcher.group(1);
            fullTestClassName = matcher.group(1);
        } else {
            fullTestClassName = description.getDisplayName();
        }
        return fullTestClassName;
    }

    public String getTestCaseName(Description description) {
        String baseTestClassName;
        Pattern pattern = Pattern.compile("(.*)\\(");
        Matcher matcher = pattern.matcher(description.getDisplayName());
        if (matcher.find()) {
            baseTestClassName = matcher.group(1);
        } else {
            baseTestClassName = description.getDisplayName();
        }
        return baseTestClassName;
    }

    public String getFormattedDuration(long duration) {
        float floatDuration = duration / 1000.0f;
        return String.format("%.4f", floatDuration);
    }

    public TestSuiteResultsStore getStore(Description description) {
        TestSuiteResultsStore store;
        // check if suite already exists, if not, create it
        if (!testSuitesMap.containsKey(getTestSuiteClassName(description))) {
            store = new TestSuiteResultsStore();
            testSuitesMap.put(getTestSuiteClassName(description), store);
        } else {
            store = testSuitesMap.get(getTestSuiteClassName(description));
        }
        return store;
    }

    // the following methods are called by JUnit framework

    // called when an atomic test flags that it assumes a condition
    // that is false
    @Override
    public void testAssumptionFailure(Failure failure) {
        logger.info(">>> testAssumptionFailure called");
        Description description = failure.getDescription();
        printDescription(description, 1);
        // should have been created already in previous call to testStarted
        TestSuiteResultsStore store = getStore(description);
        store.errors++;
        logger.info("<<< testAssumptionFailure called");
    }

    // called when an atomic test fails
    @Override
    public void testFailure(Failure failure) {
        logger.info(">>> testFailure called");

        Description description = failure.getDescription();
        printDescription(description, 1);
        logger.info("- failure.getException() = " + failure.getException());
        logger.info("- failure.getMessage() = " + failure.getMessage());
        logger.info("- failure.getTestHeader() = " + failure.getTestHeader());
        logger.info("- failure.getTrace() = " + failure.getTrace());

        if (description == null || description.getDisplayName() == null) {
            logger.info("<<< testFinished called");
            return;
        }

        // should have been created already in previous call to testStarted
        TestSuiteResultsStore store = getStore(description);

        TestCaseFailure testCaseFailure = new TestCaseFailure(failure.getMessage(), // message
                                                              failure.getException().toString(), // type
                                                              failure.getTrace() // text
        );
        TestCaseResult testCaseResult = new TestCaseResult(getTestCaseName(description),
                                                           getTestSuiteClassName(description),
                                                           null, // test not finished yet, will be updated later
                                                           "failed", // status
                                                           testCaseFailure, // failure
                                                           null // no systemOut
        );
        store.testCaseResults.add(testCaseResult);
        store.failures++;
        // everything else will be done in testFinished, which is also
        // called for failed tests as well.

        logger.info("<<< testFailure called");
    }

    // called when an atomic test is to be started
    @Override
    public void testStarted(Description description) {
        // A Description describes a test which is to be run or has been run. Descriptions can be atomic (a single test) or compound (containing children tests).
        logger.info(">>> testStarted called");

        // assume that one tests runs at a time.
        // it is then sufficient to store a global start time instead of a
        // test case specific start time at this point
        startTimeTestCase = System.currentTimeMillis();

        printDescription(description, 1);
        logger.info("<<< testStarted called");
    }

    // called when a test has finished, whether the test succeeds or fails
    @Override
    public void testFinished(Description description) {
        logger.info(">>> testFinished called");
        printDescription(description, 1);

        if (description == null || description.getDisplayName() == null) {
            logger.info("<<< testFinished called");
            return;
        }

        TestSuiteResultsStore store = getStore(description);

        long endTimeTestCase = System.currentTimeMillis();

        // this case may have failed and then we already have an entry.
        // check for description in list
        boolean found = false;
        for (int i = 0; i < store.testCaseResults.size(); i++) {
            if (store.testCaseResults.get(i).getName().equals(getTestCaseName(description))) {
                // update time for failed entry
                //testCaseResults.get(i).setTime(Long.valueOf(endTimeTestCase - startTimeTestCase).toString());
                store.testCaseResults.get(i).setTime(getFormattedDuration(endTimeTestCase - startTimeTestCase));
                found = true;
                break;
            }
        }
        if (found == false) {
            // create new entry
            TestCaseResult testCaseResult = new TestCaseResult(getTestCaseName(description),
                                                               getTestSuiteClassName(description),
                                                               //Long.valueOf(endTimeTestCase - startTimeTestCase).toString(),
                                                               getFormattedDuration(endTimeTestCase
                                                                       - startTimeTestCase),
                                                               "ok", // status
                                                               null, // no failure
                                                               null // no systemOut
            );
            store.testCaseResults.add(testCaseResult);
        }
        store.tests++;
        store.consumedTime += (endTimeTestCase - startTimeTestCase);
        logger.info("<<< testFinished called");
    }

    // called when a test will not run due to annotation @Ignore
    @Override
    public void testIgnored(Description description) {
        logger.info(">>> testIgnored called");
        printDescription(description, 1);
        if (description == null || description.getDisplayName() == null) {
            logger.info("<<< testFinished called");
            return;
        }

        TestSuiteResultsStore store = getStore(description);
        // create new entry
        TestCaseResult testCaseResult = new TestCaseResult(getTestCaseName(description),
                                                           getTestSuiteClassName(description),
                                                           //Long.valueOf(endTimeTestCase - startTimeTestCase).toString(),
                                                           "0.000",
                                                           "ignored", // status
                                                           null, // no failure
                                                           null // no systemOut
        );
        store.testCaseResults.add(testCaseResult);
        store.skipped++;

        logger.info("<<< testIgnored called");
    }

    // called when all tests are finished
    @Override
    public void testRunFinished(Result result) {
        ArrayList<TestSuiteResult> testSuiteResults = new ArrayList<TestSuiteResult>();

        logger.info(">>> testRunFinished called");

        logger.info("testRunFinished  testSuitesMap.size() = " + testSuitesMap.size());
        for (Map.Entry<String, TestSuiteResultsStore> testSuiteEntry : testSuitesMap.entrySet()) {
            logger.info("testRunFinished testSuiteName = " + testSuiteEntry.getKey());
            TestSuiteResultsStore store = testSuiteEntry.getValue();
            TestSuiteResult testSuiteResult = new TestSuiteResult(testSuiteEntry.getKey(),
                                                                  getFormattedDuration(store.consumedTime),
                                                                  store.tests,
                                                                  store.errors,
                                                                  store.skipped,
                                                                  store.failures,
                                                                  store.testCaseResults.toArray(new TestCaseResult[store.testCaseResults.size()]));

            testSuiteResults.add(testSuiteResult);
        }
        logger.info("testRunFinished  after for loop");

        testResult = new TestResult(testSuiteResults.toArray(new TestSuiteResult[testSuiteResults.size()]));

        logger.info("<<< testRunFinished called");
    }

    // called before any tests have been run
    @Override
    public void testRunStarted(Description description) {
        logger.info(">>> testRunStarted called");
        printDescription(description, 1);
        logger.info("<<< testRunStarted called");
    }
}
