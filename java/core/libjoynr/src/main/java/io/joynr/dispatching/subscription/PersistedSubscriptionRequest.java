package io.joynr.dispatching.subscription;

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

import java.io.Serializable;

import joynr.SubscriptionRequest;

public class PersistedSubscriptionRequest implements Serializable {
    private static final long serialVersionUID = 1L;
    SubscriptionRequest subscriptonRequest;
    String providerParticipantId;
    private String proxyParticipantId;

    public PersistedSubscriptionRequest(String proxyParticipantId,
                                        String providerParticipantId,
                                        SubscriptionRequest subscriptionRequest) {
        this.providerParticipantId = providerParticipantId;
        this.proxyParticipantId = proxyParticipantId;
        subscriptonRequest = subscriptionRequest;
    }

    public SubscriptionRequest getSubscriptonRequest() {
        return subscriptonRequest;
    }

    public void setSubscriptonRequest(SubscriptionRequest subscriptonRequest) {
        this.subscriptonRequest = subscriptonRequest;
    }

    public String getProviderParticipantId() {
        return providerParticipantId;
    }

    public void setProviderParticipantId(String subscriptionForParticipantId) {
        this.providerParticipantId = subscriptionForParticipantId;
    }

    public String getProxyParticipantId() {
        return proxyParticipantId;
    }

    public void setProxyParticipantId(String subscriptionFromParticipantId) {
        this.proxyParticipantId = subscriptionFromParticipantId;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((providerParticipantId == null) ? 0 : providerParticipantId.hashCode());
        result = prime * result + ((proxyParticipantId == null) ? 0 : proxyParticipantId.hashCode());
        result = prime * result + ((subscriptonRequest == null) ? 0 : subscriptonRequest.hashCode());
        return result;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (obj == null)
            return false;
        if (getClass() != obj.getClass())
            return false;
        PersistedSubscriptionRequest other = (PersistedSubscriptionRequest) obj;
        if (providerParticipantId == null) {
            if (other.providerParticipantId != null)
                return false;
        } else if (!providerParticipantId.equals(other.providerParticipantId))
            return false;
        if (proxyParticipantId == null) {
            if (other.proxyParticipantId != null)
                return false;
        } else if (!proxyParticipantId.equals(other.proxyParticipantId))
            return false;
        if (subscriptonRequest == null) {
            if (other.subscriptonRequest != null)
                return false;
        } else if (!subscriptonRequest.equals(other.subscriptonRequest))
            return false;
        return true;
    }
}
