package joynr.exceptions;

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

import com.fasterxml.jackson.annotation.JsonProperty;

import io.joynr.exceptions.JoynrRuntimeException;

/**
 * Joynr exception to report missed periodic publications.
 */
public class PublicationMissedException extends JoynrRuntimeException {
    private static final long serialVersionUID = 1L;

    @JsonProperty
    private String subscriptionId;

    /**
     * Constructor for a PublicationMissedException with subscription ID.
     *
     * @param subscriptionId the subscription ID of the subscription the missed
     * publication belongs to.
     */
    public PublicationMissedException(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    /**
     * @return the subscription ID of the subscription the missed publication
     * belongs to.
     */
    public String getSubscriptionId() {
        return subscriptionId;
    }

    @Override
    public String toString() {
        return super.toString() + ": subscriptionId=" + subscriptionId;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((subscriptionId == null) ? 0 : subscriptionId.hashCode());
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
        PublicationMissedException other = (PublicationMissedException) obj;
        if (subscriptionId == null) {
            if (other.subscriptionId != null) {
                return false;
            }
        } else if (!subscriptionId.equals(other.subscriptionId)) {
            return false;
        }
        return true;
    }
}
