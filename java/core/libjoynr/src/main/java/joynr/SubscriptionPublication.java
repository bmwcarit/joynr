package joynr;

import io.joynr.exceptions.JoynrException;

import java.util.List;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

public class SubscriptionPublication implements JoynrMessageType {

    private static final long serialVersionUID = 1L;

    private String subscriptionId;
    private List<? extends Object> response;
    private JoynrException error;

    public SubscriptionPublication() {
    }

    public SubscriptionPublication(List<? extends Object> response, String subscriptionId) {
        this.response = response;
        this.error = null;
        this.subscriptionId = subscriptionId;
    }

    public SubscriptionPublication(JoynrException error, String subscriptionId) {
        this.error = error;
        this.response = null;
        this.subscriptionId = subscriptionId;
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }

    public void setSubscriptionId(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    public Object getResponse() {
        return response;
    }

    public JoynrException getError() {
        return error;
    }

    @Override
    public String toString() {
        return "SubscriptionPublication [" + "subscriptionId=" + subscriptionId + ", "
                + (response != null ? "response=" + response + ", " : "") + (error != null ? "error=" + error : "")
                + "]";
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
        SubscriptionPublication other = (SubscriptionPublication) obj;
        if (error == null) {
            if (other.error != null) {
                return false;
            }
        } else if (!error.equals(other.error)) {
            return false;
        }
        if (response == null) {
            if (other.response != null) {
                return false;
            }
        } else if (!response.equals(other.response)) {
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
        result = prime * result + ((response == null) ? 0 : response.hashCode());
        result = prime * result + ((subscriptionId == null) ? 0 : subscriptionId.hashCode());
        return result;
    }
}
