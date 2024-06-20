/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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
package io.joynr.exceptions;

import joynr.types.DiscoveryError;

/**
 * Joynr exception to report errors during discovery.
 */
public class DiscoveryException extends JoynrRuntimeException {
    private static final long serialVersionUID = -5333394188396893835L;

    private final Enum<DiscoveryError> error;

    /**
     * Constructor for a DiscoveryException with detail message.
     *
     * @param message further description of the reported discovery error
     */
    public DiscoveryException(String message) {
        super(message);
        error = null;
    }

    /**
     * Constructor for a DiscoveryException with detail message and
     * discovery error enum.
     *
     * @param message further description of the reported discovery error
     * @param error discovery error enum
     */
    public DiscoveryException(String message, DiscoveryError error) {
        super(message);
        this.error = error;
    }

    public DiscoveryException(Throwable cause) {
        super(cause);
        this.error = null;
    }

    public Enum<DiscoveryError> getError() {
        return error;
    }

    @Override
    public String toString() {
        return this.getClass().getName() + ", ErrorValue: " + this.error + ", Message: " + this.getMessage();
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (obj == null) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        DiscoveryException other = (DiscoveryException) obj;
        if (getMessage() == null) {
            if (other.getMessage() != null) {
                return false;
            }
        } else if (!getMessage().equals(other.getMessage())) {
            return false;
        }
        if (error == null) {
            return other.error == null;
        } else
            return error.equals(other.error);
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((getMessage() == null) ? 0 : getMessage().hashCode());
        result = prime * result + ((error == null) ? 0 : error.hashCode());
        return result;
    }
}
