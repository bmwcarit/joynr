/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import jakarta.ejb.ApplicationException;

import com.fasterxml.jackson.annotation.JsonProperty;

@ApplicationException
public class JoynrRuntimeException extends RuntimeException implements JoynrException {
    private static final long serialVersionUID = 1L;

    public JoynrRuntimeException() {
        super();
    }

    public JoynrRuntimeException(String message) {
        super(message);
    }

    public JoynrRuntimeException(String message, Throwable cause) {
        super(message, cause);
    }

    public JoynrRuntimeException(Throwable cause) {
        super(cause);
    }

    @JsonProperty("detailMessage")
    @Override
    public String getMessage() {
        return super.getMessage();
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
        JoynrRuntimeException other = (JoynrRuntimeException) obj;
        if (getMessage() == null) {
            if (other.getMessage() != null) {
                return false;
            }
        } else if (!getMessage().equals(other.getMessage())) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((getMessage() == null) ? 0 : getMessage().hashCode());
        return result;
    }
}
