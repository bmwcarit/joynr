/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.test.interlanguage.testresults;

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
public class TestSuiteResult implements Serializable, JoynrType {
    public static final int MAJOR_VERSION = 0;
    public static final int MINOR_VERSION = 0;
    @JsonProperty("name")
    private String name;
    @JsonProperty("time")
    private String time;
    @JsonProperty("tests")
    private Integer tests;
    @JsonProperty("errors")
    private Integer errors;
    @JsonProperty("skipped")
    private Integer skipped;
    @JsonProperty("failures")
    private Integer failures;
    @JsonProperty("testCaseResults")
    private TestCaseResult[] testCaseResults = {};

    /**
     * Default Constructor
     */
    public TestSuiteResult() {
        this.name = "";
        this.time = "";
        this.tests = 0;
        this.errors = 0;
        this.skipped = 0;
        this.failures = 0;
    }

    /**
     * Copy constructor
     *
     * @param testSuiteResultObj reference to the object to be copied
     */
    public TestSuiteResult(TestSuiteResult testSuiteResultObj) {
        this.name = testSuiteResultObj.name;
        this.time = testSuiteResultObj.time;
        this.tests = testSuiteResultObj.tests;
        this.errors = testSuiteResultObj.errors;
        this.skipped = testSuiteResultObj.skipped;
        this.failures = testSuiteResultObj.failures;
        this.testCaseResults = testSuiteResultObj.testCaseResults;
    }

    /**
     * Parameterized constructor
     *
     * @param name
     * @param time
     * @param tests
     * @param errors
     * @param skipped
     * @param failures
     * @param testCaseResults
     */
    public TestSuiteResult(String name,
                           String time,
                           Integer tests,
                           Integer errors,
                           Integer skipped,
                           Integer failures,
                           TestCaseResult[] testCaseResults) {
        this.name = name;
        this.time = time;
        this.tests = tests;
        this.errors = errors;
        this.skipped = skipped;
        this.failures = failures;
        this.testCaseResults = testCaseResults == null ? null : testCaseResults.clone();
    }

    /**
     * Gets Name
     *
     * @return
     */
    @JsonIgnore
    public String getName() {
        return this.name;
    }

    /**
     * Gets Time
     *
     * @return
     */
    @JsonIgnore
    public String getTime() {
        return this.time;
    }

    /**
     * Gets Tests
     *
     * @return
     */
    @JsonIgnore
    public Integer getTests() {
        return this.tests;
    }

    /**
     * Gets Errors
     *
     * @return
     */
    @JsonIgnore
    public Integer getErrors() {
        return this.errors;
    }

    /**
     * Gets Skipped
     *
     * @return
     */
    @JsonIgnore
    public Integer getSkipped() {
        return this.skipped;
    }

    /**
     * Gets Failures
     *
     * @return
     */
    @JsonIgnore
    public Integer getFailures() {
        return this.failures;
    }

    /**
     * Gets TestCaseResults
     *
     * @return
     */
    @JsonIgnore
    public TestCaseResult[] getTestCaseResults() {
        return testCaseResults == null ? null : testCaseResults.clone();
    }

    /**
     * Stringifies the class
     *
     * @return stringified class content
     */
    @Override
    public String toString() {
        return "TestSuiteResult [" + "name=" + this.name + ", " + "time=" + this.time + ", " + "tests=" + this.tests
                + ", " + "errors=" + this.errors + ", " + "skipped=" + this.skipped + ", " + "failures=" + this.failures
                + ", " + "testCaseResults=" + java.util.Arrays.toString(this.testCaseResults) + "]";
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
        TestSuiteResult other = (TestSuiteResult) obj;
        if (this.name == null) {
            if (other.name != null) {
                return false;
            }
        } else if (!this.name.equals(other.name)) {
            return false;
        }
        if (this.time == null) {
            if (other.time != null) {
                return false;
            }
        } else if (!this.time.equals(other.time)) {
            return false;
        }
        if (this.tests == null) {
            if (other.tests != null) {
                return false;
            }
        } else if (!this.tests.equals(other.tests)) {
            return false;
        }
        if (this.errors == null) {
            if (other.errors != null) {
                return false;
            }
        } else if (!this.errors.equals(other.errors)) {
            return false;
        }
        if (this.skipped == null) {
            if (other.skipped != null) {
                return false;
            }
        } else if (!this.skipped.equals(other.skipped)) {
            return false;
        }
        if (this.failures == null) {
            if (other.failures != null) {
                return false;
            }
        } else if (!this.failures.equals(other.failures)) {
            return false;
        }
        if (this.testCaseResults == null) {
            if (other.testCaseResults != null) {
                return false;
            }
        } else if (!java.util.Arrays.deepEquals(this.testCaseResults, other.testCaseResults)) {
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
        result = prime * result + ((this.name == null) ? 0 : this.name.hashCode());
        result = prime * result + ((this.time == null) ? 0 : this.time.hashCode());
        result = prime * result + ((this.tests == null) ? 0 : this.tests.hashCode());
        result = prime * result + ((this.errors == null) ? 0 : this.errors.hashCode());
        result = prime * result + ((this.skipped == null) ? 0 : this.skipped.hashCode());
        result = prime * result + ((this.failures == null) ? 0 : this.failures.hashCode());
        result = prime * result
                + ((this.testCaseResults == null) ? 0 : java.util.Arrays.hashCode(this.testCaseResults));
        return result;
    }
}
