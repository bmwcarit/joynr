package joynr;

/*
 * #%L
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

import io.joynr.pubsub.SubscriptionQos;

public class SubscriptionRequest implements JoynrMessageType {

    private String subscriptionId;
    private String subscribedToName;
    private SubscriptionQos qos;

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

}
