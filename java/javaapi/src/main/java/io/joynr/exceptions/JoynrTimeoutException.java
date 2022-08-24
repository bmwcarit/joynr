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
package io.joynr.exceptions;

import java.text.SimpleDateFormat;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.deser.std.StdDeserializer;

public class JoynrTimeoutException extends JoynrRuntimeException {
    private static final long serialVersionUID = 1L;

    @JsonProperty
    private long expiryDate;

    /**
     * DO NOT USE
     * Constructor for deserializer
     */
    public JoynrTimeoutException(String message, long expiryDate, StdDeserializer<JoynrTimeoutException> deserializer) {
        super(message);
        this.expiryDate = expiryDate;
    }

    public JoynrTimeoutException(long expiryDate) {
        super("ttl expired on: " + (new SimpleDateFormat("dd/MM HH:mm:ss:sss")).format(expiryDate));
        this.expiryDate = expiryDate;
    }

    public JoynrTimeoutException(long expiryDate, String requestReplyId) {
        super("ttl for request with requestReplyId " + requestReplyId + " expired on: "
                + (new SimpleDateFormat("dd/MM HH:mm:ss:sss")).format(expiryDate));
        this.expiryDate = expiryDate;
    }

    public long getExpiryDate() {
        return expiryDate;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + (int) (expiryDate ^ (expiryDate >>> 32));
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }
        if (!super.equals(obj)) {
            return false;
        }
        if (getClass() != obj.getClass()) {
            return false;
        }
        JoynrTimeoutException other = (JoynrTimeoutException) obj;
        if (expiryDate != other.expiryDate) {
            return false;
        }
        return true;
    }

}
