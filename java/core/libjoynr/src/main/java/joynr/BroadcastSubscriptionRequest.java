package joynr;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import java.util.Map;

public class BroadcastSubscriptionRequest extends SubscriptionRequest {

    private Map<String, Object> filterParameters;

    /**
     * \class BroadcastSubscriptionRequest \brief BroadcastSubscriptionRequest stores the information that is necessary
     * to store a a slective broadcast request on subscriber side, while Arbitration is handled.
     */

    public BroadcastSubscriptionRequest() {
        super();
    }

    public BroadcastSubscriptionRequest(String subscriptionId, String subscribedToName, SubscriptionQos qos) {
        super(subscriptionId, subscribedToName, qos);
    }

    @Override
    public String toString() {
        return "BroadcastSubscriptionRequest [subscriptionId=" + subscriptionId + "," + ", subscribedToName="
                + subscribedToName + ", filterParameters=" + filterParameters + "]";
    }

    @Override
    public void setQos(final SubscriptionQos qos) {
        if (qos instanceof OnChangeSubscriptionQos) {
            super.setQos(qos);
        } else {
            throw new IllegalArgumentException("only OnChangeSubscriptionQos is supported for selective broadcasts");
        }
    }

    public Map<String, Object> getFilterParameters() {
        return filterParameters;
    }

    public void setFilterParameters(final Map<String, Object> filterParameters) {
        this.filterParameters = filterParameters;
    }

}
