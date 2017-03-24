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

public class MulticastSubscriptionRequest extends SubscriptionRequest {

    private static final long serialVersionUID = 1L;

    private String multicastId;

    public MulticastSubscriptionRequest(String multicastId,
                                        String subscriptionId,
                                        String multicastName,
                                        SubscriptionQos qos) {
        super(subscriptionId, multicastName, qos);
        this.multicastId = multicastId;
    }

    public MulticastSubscriptionRequest() {
    }

    public String getMulticastId() {
        return multicastId;
    }

    public void setMulticastId(String multicastId) {
        this.multicastId = multicastId;
    }

    @Override
    public boolean equals(Object o) {
        if (this == o)
            return true;
        if (o == null || getClass() != o.getClass())
            return false;
        if (!super.equals(o))
            return false;

        MulticastSubscriptionRequest that = (MulticastSubscriptionRequest) o;

        return multicastId.equals(that.multicastId);
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + multicastId.hashCode();
        return result;
    }

    @Override
    public String toString() {
        return "MulticastSubscriptionRequest [" + "multicastId='" + multicastId + "', subscriptionId='"
                + getSubscriptionId() + "', subscribedToName='" + getSubscribedToName() + "', qos='" + getQos() + "']";
    }
}
