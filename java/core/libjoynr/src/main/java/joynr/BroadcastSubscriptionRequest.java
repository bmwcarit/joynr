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
package joynr;

import io.joynr.pubsub.SubscriptionQos;

import java.util.HashMap;

public class BroadcastSubscriptionRequest extends SubscriptionRequest {

    /**
     *
     */
    private static final long serialVersionUID = 1L;
    private BroadcastFilterParameters filterParameters = new BroadcastFilterParameters();

    /**
     * \class BroadcastSubscriptionRequest \brief BroadcastSubscriptionRequest stores the information that is necessary
     * to store a a selective broadcast request on subscriber side, while Arbitration is handled.
     */

    public BroadcastSubscriptionRequest() {
        super();
    }

    public BroadcastSubscriptionRequest(String subscriptionId,
                                        String subscribedToName,
                                        BroadcastFilterParameters filterParameters,
                                        OnChangeSubscriptionQos qos) {
        super(subscriptionId, subscribedToName, qos);
        if (filterParameters != null) {
            this.filterParameters.setFilterParameters(filterParameters.getFilterParameters());
        }

    }

    public BroadcastFilterParameters getFilterParameters() {
        if (filterParameters == null) {
            return null;
        }
        BroadcastFilterParameters parameters = new BroadcastFilterParameters();
        parameters.setFilterParameters(new HashMap<>(filterParameters.getFilterParameters()));
        return parameters;
    }

    @Override
    public String toString() {
        return "BroadcastSubscriptionRequest [subscriptionId=" + subscriptionId + "," + ", subscribedToName="
                + subscribedToName + ", filterParameters=" + filterParameters + "]";
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((filterParameters == null) ? 0 : filterParameters.hashCode());
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
        BroadcastSubscriptionRequest other = (BroadcastSubscriptionRequest) obj;
        if (filterParameters == null) {
            if (other.filterParameters != null) {
                return false;
            }
        } else if (!filterParameters.equals(other.filterParameters)) {
            return false;
        }
        return true;
    }

    @Override
    public void setQos(SubscriptionQos qos) {
        if (!(qos instanceof OnChangeSubscriptionQos)) {
            throw new IllegalArgumentException("Qos of broadcast subsription is expected of type "
                    + OnChangeSubscriptionQos.class.getSimpleName());
        }
        super.setQos(qos);
    }
}
