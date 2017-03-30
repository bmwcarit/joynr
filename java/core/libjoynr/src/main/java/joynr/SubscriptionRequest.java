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

import io.joynr.pubsub.SubscriptionQos;

public class SubscriptionRequest implements JoynrMessageType {
    private static final long serialVersionUID = 1L;
    protected String subscriptionId;
    protected String subscribedToName;
    protected SubscriptionQos qos;

    /**
     * \class SubscriptionRequest \brief SubscriptionRequest stores the information that is necessary to store a
     * subscription-Request on subscriber side, while Arbitration is handled.
     */

    public SubscriptionRequest() {
    }

    public SubscriptionRequest(String subscriptionId, String subscribedToName, SubscriptionQos qos) {
        super();
        this.subscriptionId = subscriptionId;
        this.subscribedToName = subscribedToName;
        this.qos = qos;
    }

    public String getSubscriptionId() {
        return subscriptionId;
    }

    public void setSubscriptionId(String subscriptionId) {
        this.subscriptionId = subscriptionId;
    }

    public String getSubscribedToName() {
        return subscribedToName;
    }

    public void setSubscribedToName(String subscribedToName) {
        this.subscribedToName = subscribedToName;
    }

    public SubscriptionQos getQos() {
        return qos;
    }

    public void setQos(SubscriptionQos qos) {
        this.qos = qos;
    }

    @Override
    public String toString() {
        return "SubscriptionRequest [subscriptionId=" + subscriptionId + "," + ", subscribedToName=" + subscribedToName
                + "]";
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((qos == null) ? 0 : qos.hashCode());
        result = prime * result + ((subscribedToName == null) ? 0 : subscribedToName.hashCode());
        result = prime * result + ((subscriptionId == null) ? 0 : subscriptionId.hashCode());
        return result;
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
        SubscriptionRequest other = (SubscriptionRequest) obj;
        if (qos == null) {
            if (other.qos != null) {
                return false;
            }
        } else if (!qos.equals(other.qos)) {
            return false;
        }
        if (subscribedToName == null) {
            if (other.subscribedToName != null) {
                return false;
            }
        } else if (!subscribedToName.equals(other.subscribedToName)) {
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

}
