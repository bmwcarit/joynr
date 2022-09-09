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
package joynr.exceptions;

import com.fasterxml.jackson.annotation.JsonProperty;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.types.Version;

/**
 * Joynr exception class to report error during method invocations (RPC) at a provider
 * ("no such method", invalid arguments, etc.)
 */
public class MethodInvocationException extends JoynrRuntimeException {
    private static final long serialVersionUID = 2077255751860166967L;
    @JsonProperty("providerVersion")
    private Version providerVersion;

    /**
     * Constructor for a MethodInvocationException with detail message.
     *
     * @param message further description of the reported invocation error
     */
    public MethodInvocationException(String message) {
        super(message);
    }

    /**
     * Constructor for a MethodInvocationException with detail message.
     *
     * @param cause exception that caused the method invocation exception
     */
    public MethodInvocationException(Exception cause) {
        super(cause);
    }

    /**
     * Constructor for a MethodInvocationException with detail message.
     *
     * @param message further description of the reported invocation error
     * @param providerVersion the version of the provider which could not handle the method invocation
     */
    public MethodInvocationException(String message, Version providerVersion) {
        super(message);
        this.providerVersion = providerVersion;
    }

    /**
     * Constructor for a MethodInvocationException with detail message.
     *
     * @param cause exception that caused the method invocation exception
     * @param providerVersion the version of the provider which could not handle the method invocation
     */
    public MethodInvocationException(Exception cause, Version providerVersion) {
        super(cause);
        this.providerVersion = providerVersion;
    }

    /**
     * Gets the version of the provider which could not handle the method invocation
     *
     * @return the version of the provider which could not handle the method invocation
     */
    public Version getProviderVersion() {
        return providerVersion;
    }

    @Override
    public String toString() {
        return super.toString() + (providerVersion == null ? "" : ", providerVersion: " + providerVersion);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        MethodInvocationException other = (MethodInvocationException) obj;
        if (getProviderVersion() == null) {
            if (other.getProviderVersion() != null) {
                return false;
            }
        } else if (!getProviderVersion().equals(other.getProviderVersion())) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((providerVersion == null) ? 0 : providerVersion.hashCode());
        return result;
    }
}
