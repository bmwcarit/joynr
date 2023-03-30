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
package io.joynr.test.gcd.testresults;

import java.io.Serializable;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import io.joynr.subtypes.JoynrType;

// NOTE: serialVersionUID is not defined since we don't support Franca versions right now.
//       The compiler will generate a serialVersionUID based on the class and its members
//       (cf. http://docs.oracle.com/javase/6/docs/platform/serialization/spec/class.html#4100),
//       which is probably more restrictive than what we want.

/**
 */
@SuppressWarnings("serial")
public class TestResult implements Serializable, JoynrType {
    public static final int MAJOR_VERSION = 0;
    public static final int MINOR_VERSION = 0;
    @JsonProperty("testSuiteResults")
    private TestSuiteResult[] testSuiteResults = {};

    /**
     * Default Constructor
     */
    public TestResult() {
    }

    /**
     * Copy constructor
     *
     * @param testResultObj reference to the object to be copied
     */
    public TestResult(TestResult testResultObj) {
        this.testSuiteResults = testResultObj.testSuiteResults;
    }

    /**
     * Parameterized constructor
     *
     * @param testSuiteResults
     */
    public TestResult(TestSuiteResult[] testSuiteResults) {
        this.testSuiteResults = testSuiteResults == null ? null : testSuiteResults.clone();
    }

    /**
     * Gets TestSuiteResults
     *
     * @return
     */
    @JsonIgnore
    public TestSuiteResult[] getTestSuiteResults() {
        return testSuiteResults == null ? null : testSuiteResults.clone();
    }

    /**
     * Stringifies the class
     *
     * @return stringified class content
     */
    @Override
    public String toString() {
        return "TestResult [" + "testSuiteResults=" + java.util.Arrays.toString(this.testSuiteResults) + "]";
    }

    /**
     * Check for equality
     *
     * @param obj Reference to the object to compare to
     * @return true, if objects are equal, false otherwise
     */
    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        TestResult other = (TestResult) obj;
        if (this.testSuiteResults == null) {
            if (other.testSuiteResults != null) {
                return false;
            }
        } else if (!java.util.Arrays.deepEquals(this.testSuiteResults, other.testSuiteResults)) {
            return false;
        }
        return true;
    }

    /**
     * Calculate code for hashing based on member contents
     *
     * @return The calculated hash code
     */
    @Override
    public int hashCode() {
        int result = 1;
        final int prime = 31;
        result = prime * result
                + ((this.testSuiteResults == null) ? 0 : java.util.Arrays.hashCode(this.testSuiteResults));
        return result;
    }
}
