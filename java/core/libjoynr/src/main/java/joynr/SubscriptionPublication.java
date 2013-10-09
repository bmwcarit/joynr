package joynr;

/*
 * #%L
 * joynr::java::core::libjoynr
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

    private String subscriptionId;
    private Object response;

    public SubscriptionPublication() {
    }

    public SubscriptionPublication(Object response, String subscriptionId) {
        this.response = response;
        this.subscriptionId = subscriptionId;
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }

    public void setSubscriptionId(String subscriptionId) {
        this.subscriptionId = subscriptionId;

    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        SubscriptionPublication other = (SubscriptionPublication) obj;

        if (response == null) {
            if (other.response != null) {
                return false;
            }
        } else if (!response.equals(other.getResponse())) {
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

    public Object getResponse() {
        return response;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((this.response == null) ? 0 : response.hashCode());
        result = prime * result + ((this.subscriptionId == null) ? 0 : subscriptionId.hashCode());
        return result;
    }

}
