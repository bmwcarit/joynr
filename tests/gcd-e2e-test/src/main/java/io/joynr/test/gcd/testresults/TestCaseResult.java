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
public class TestCaseResult implements Serializable, JoynrType {
    public static final int MAJOR_VERSION = 0;
    public static final int MINOR_VERSION = 0;
    @JsonProperty("name")
    private String name;
    @JsonProperty("classname")
    private String classname;
    @JsonProperty("time")
    private String time;
    @JsonProperty("status")
    private String status;
    @JsonProperty("failure")
    private TestCaseFailure failure;
    @JsonProperty("systemOut")
    private String systemOut;

    /**
     * Default Constructor
     */
    public TestCaseResult() {
        this.name = "";
        this.classname = "";
        this.time = "";
        this.status = "";
        this.failure = new TestCaseFailure();
        this.systemOut = "";
    }

    /**
     * Copy constructor
     *
     * @param testCaseResultObj reference to the object to be copied
     */
    public TestCaseResult(TestCaseResult testCaseResultObj) {
        this.name = testCaseResultObj.name;
        this.classname = testCaseResultObj.classname;
        this.time = testCaseResultObj.time;
        this.status = testCaseResultObj.status;
        this.failure = new TestCaseFailure(testCaseResultObj.failure);
        this.systemOut = testCaseResultObj.systemOut;
    }

    /**
     * Parameterized constructor
     *
     * @param name
     * @param classname
     * @param time
     * @param status
     * @param failure
     * @param systemOut
     */
    public TestCaseResult(String name,
                          String classname,
                          String time,
                          String status,
                          TestCaseFailure failure,
                          String systemOut) {
        this.name = name;
        this.classname = classname;
        this.time = time;
        this.status = status;
        this.failure = failure;
        this.systemOut = systemOut;
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
     * Gets Classname
     *
     * @return
     */
    @JsonIgnore
    public String getClassname() {
        return this.classname;
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
     * Sets Time
     *
     * @param time
     */
    @JsonIgnore
    public void setTime(String time) {
        if (time == null) {
            throw new IllegalArgumentException("setting time to null is not allowed");
        }
        this.time = time;
    }

    /**
     * Gets Status
     *
     * @return
     */
    @JsonIgnore
    public String getStatus() {
        return this.status;
    }

    /**
     * Gets Failure
     *
     * @return
     */
    @JsonIgnore
    public TestCaseFailure getFailure() {
        return this.failure;
    }

    /**
     * Gets SystemOut
     *
     * @return
     */
    @JsonIgnore
    public String getSystemOut() {
        return this.systemOut;
    }

    /**
     * Stringifies the class
     *
     * @return stringified class content
     */
    @Override
    public String toString() {
        return "TestCaseResult [" + "name=" + this.name + ", " + "classname=" + this.classname + ", " + "time="
                + this.time + ", " + "status=" + this.status + ", " + "failure=" + this.failure + ", " + "systemOut="
                + this.systemOut + "]";
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
        TestCaseResult other = (TestCaseResult) obj;
        if (this.name == null) {
            if (other.name != null) {
                return false;
            }
        } else if (!this.name.equals(other.name)) {
            return false;
        }
        if (this.classname == null) {
            if (other.classname != null) {
                return false;
            }
        } else if (!this.classname.equals(other.classname)) {
            return false;
        }
        if (this.time == null) {
            if (other.time != null) {
                return false;
            }
        } else if (!this.time.equals(other.time)) {
            return false;
        }
        if (this.status == null) {
            if (other.status != null) {
                return false;
            }
        } else if (!this.status.equals(other.status)) {
            return false;
        }
        if (this.failure == null) {
            if (other.failure != null) {
                return false;
            }
        } else if (!this.failure.equals(other.failure)) {
            return false;
        }
        if (this.systemOut == null) {
            if (other.systemOut != null) {
                return false;
            }
        } else if (!this.systemOut.equals(other.systemOut)) {
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
        result = prime * result + ((this.classname == null) ? 0 : this.classname.hashCode());
        result = prime * result + ((this.time == null) ? 0 : this.time.hashCode());
        result = prime * result + ((this.status == null) ? 0 : this.status.hashCode());
        result = prime * result + ((this.failure == null) ? 0 : this.failure.hashCode());
        result = prime * result + ((this.systemOut == null) ? 0 : this.systemOut.hashCode());
        return result;
    }
}
