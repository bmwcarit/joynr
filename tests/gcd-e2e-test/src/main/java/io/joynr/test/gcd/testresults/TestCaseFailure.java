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
public class TestCaseFailure implements Serializable, JoynrType {
    public static final int MAJOR_VERSION = 0;
    public static final int MINOR_VERSION = 0;
    @JsonProperty("message")
    private String message;
    @JsonProperty("type")
    private String type;
    @JsonProperty("text")
    private String text;

    /**
     * Default Constructor
     */
    public TestCaseFailure() {
        this.message = "";
        this.type = "";
        this.text = "";
    }

    /**
     * Copy constructor
     *
     * @param testCaseFailureObj reference to the object to be copied
     */
    public TestCaseFailure(TestCaseFailure testCaseFailureObj) {
        this.message = testCaseFailureObj.message;
        this.type = testCaseFailureObj.type;
        this.text = testCaseFailureObj.text;
    }

    /**
     * Parameterized constructor
     *
     * @param message
     * @param type
     * @param text
     */
    public TestCaseFailure(String message, String type, String text) {
        this.message = message;
        this.type = type;
        this.text = text;
    }

    /**
     * Gets Message
     *
     * @return
     */
    @JsonIgnore
    public String getMessage() {
        return this.message;
    }

    /**
     * Gets Type
     *
     * @return
     */
    @JsonIgnore
    public String getType() {
        return this.type;
    }

    /**
     * Gets Text
     *
     * @return
     */
    @JsonIgnore
    public String getText() {
        return this.text;
    }

    /**
     * Stringifies the class
     *
     * @return stringified class content
     */
    @Override
    public String toString() {
        return "TestCaseFailure [" + "message=" + this.message + ", " + "type=" + this.type + ", " + "text=" + this.text
                + "]";
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
        TestCaseFailure other = (TestCaseFailure) obj;
        if (this.message == null) {
            if (other.message != null) {
                return false;
            }
        } else if (!this.message.equals(other.message)) {
            return false;
        }
        if (this.type == null) {
            if (other.type != null) {
                return false;
            }
        } else if (!this.type.equals(other.type)) {
            return false;
        }
        if (this.text == null) {
            if (other.text != null) {
                return false;
            }
        } else if (!this.text.equals(other.text)) {
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
        result = prime * result + ((this.message == null) ? 0 : this.message.hashCode());
        result = prime * result + ((this.type == null) ? 0 : this.type.hashCode());
        result = prime * result + ((this.text == null) ? 0 : this.text.hashCode());
        return result;
    }
}
