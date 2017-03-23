package io.joynr.exceptions;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * Thrown if an assignment of a channel to a bounce proxy instance failed.
 * 
 * @author christina.strobel
 *
 */
public class JoynrChannelNotAssignableException extends JoynrRuntimeException {

    private static final long serialVersionUID = 212450268038844695L;

    /**
     * The channel that could not be assigned to a bounce proxy.
     */
    @JsonProperty
    private String ccid;

    /**
     * Constructor for deserializer
     */
    protected JoynrChannelNotAssignableException() {
        super();
    }

    /**
     * @param message problem description
     * @param ccid the channel ID that could not be assigned.
     */
    public JoynrChannelNotAssignableException(String message, String ccid) {
        super(message + " for channel '" + ccid + "'");
        this.ccid = ccid;
    }

    public String getChannelId() {
        return ccid;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((ccid == null) ? 0 : ccid.hashCode());
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
        JoynrChannelNotAssignableException other = (JoynrChannelNotAssignableException) obj;
        if (ccid == null) {
            if (other.ccid != null) {
                return false;
            }
        } else if (!ccid.equals(other.ccid)) {
            return false;
        }
        return true;
    }
}
