/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.test.gcd;

import com.fasterxml.jackson.databind.ObjectMapper;
import org.junit.runner.JUnitCore;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.test.gcd.testresults.TestResult;
import io.joynr.test.gcd.testresults.TestSuiteResult;

public class GcdConsumerTestStarter {
    private static final Logger logger = LoggerFactory.getLogger(GcdConsumerTestStarter.class);

    public static void main(String argv[]) {
        GcdConsumerTestStarter t = new GcdConsumerTestStarter();
        boolean testSucceeded = t.runTests();
        if (!testSucceeded) {
            System.exit(1);
        }
        System.exit(0);
    }

    public boolean runTests() {
        TestResult testResult;
        boolean result;

        JUnitCore runner = new JUnitCore();
        GcdConsumerJUnitListener listener = new GcdConsumerJUnitListener();
        runner.addListener(listener);
        runner.run(GcdConsumerTestSuite.class);
        testResult = listener.getTestResult();
        ObjectMapper mapper = new ObjectMapper();
        try {
            String jsonString = mapper.writerWithDefaultPrettyPrinter().writeValueAsString(testResult);
            logger.info("runTests: serialized test results:");
            logger.info(jsonString);
        } catch (Exception e) {
            // ignore
            logger.info("runTests: failed to serialize test results");
            logger.info("runTests: TEST FAILED");
            return false;
        }

        int errors = 0;
        int failures = 0;

        for (TestSuiteResult testSuiteResult : testResult.getTestSuiteResults()) {
            errors += testSuiteResult.getErrors();
            failures += testSuiteResult.getFailures();
        }

        logger.info("runTests: #errors: " + errors + ", #failures: " + failures);
        if (errors > 0 || failures > 0) {
            logger.info("runTests: TEST FAILED");
            return false;
        }
        logger.info("runTests: TEST SUCCEEDED");
        return true;
    }
}
