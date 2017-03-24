package joynr;

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

import io.joynr.exceptions.SubscriptionException;

/**
 * Value class for the response of a subscription request.
 */
public class SubscriptionReply implements JoynrMessageType {
    private static final long serialVersionUID = 1L;
    private SubscriptionException error = null;
    private String subscriptionId = null;

    public SubscriptionReply(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    public SubscriptionReply() {
    }

    public SubscriptionReply(String subscriptionId, SubscriptionException error) {
        this.subscriptionId = subscriptionId;
        this.error = error;
    }

    public SubscriptionException getError() {
        return error;
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }

    @Override
    public String toString() {
        return "SubscriptionReply: " + "subscriptionId: " + subscriptionId + (error == null ? "" : ", error: " + error);
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
        SubscriptionReply other = (SubscriptionReply) obj;
        if (error == null) {
            if (other.error != null) {
                return false;
            }
        } else if (!error.equals(other.error)) {
            return false;
        }
        if (subscriptionId == null) {
            if (other.subscriptionId != null) {
                return false;
            }
        } else if (!subscriptionId.equals(other.subscriptionId)) {
            return false;
        }
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((error == null) ? 0 : error.hashCode());
        result = prime * result + ((subscriptionId == null) ? 0 : subscriptionId.hashCode());
        return result;
    }
}
