package io.joynr.arbitration;

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

import io.joynr.endpoints.EndpointAddressBase;

import java.util.List;

public class ArbitrationResult {
    private String participantId;
    private List<EndpointAddressBase> endpointAddress;

    public ArbitrationResult(final String participantId, final List<EndpointAddressBase> expectedEndpointAddress) {
        super();
        this.participantId = participantId;
        this.endpointAddress = expectedEndpointAddress;
    }

    public ArbitrationResult() {
    }

    public String getParticipantId() {
        return participantId;
    }

    public void setParticipantId(String participantId) {
        this.participantId = participantId;
    }

    public List<EndpointAddressBase> getEndpointAddress() {
        return endpointAddress;
    }

    public void setEndpointAddress(final List<EndpointAddressBase> endpointAddress) {
        this.endpointAddress = endpointAddress;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((endpointAddress == null) ? 0 : endpointAddress.hashCode());
        result = prime * result + ((participantId == null) ? 0 : participantId.hashCode());
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
        ArbitrationResult other = (ArbitrationResult) obj;
        if (endpointAddress == null) {
            if (other.endpointAddress != null) {
                return false;
            }
        } else if (!endpointAddress.equals(other.endpointAddress)) {
            return false;
        }
        if (participantId == null) {
            if (other.participantId != null) {
                return false;
            }
        } else if (!participantId.equals(other.participantId)) {
            return false;
        }
        return true;
    }

}
