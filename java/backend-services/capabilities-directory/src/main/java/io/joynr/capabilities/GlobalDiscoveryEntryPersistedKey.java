/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package io.joynr.capabilities;

import java.io.Serializable;

@SuppressWarnings("serial")
public class GlobalDiscoveryEntryPersistedKey implements Serializable {

    String gbid;
    String participantId;

    public String getGbid() {
        return gbid;
    }

    public void setGbid(String gbid) {
        this.gbid = gbid;
    }

    public String getParticipantId() {
        return participantId;
    }

    public void setParticipantId(String participantId) {
        this.participantId = participantId;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((gbid == null) ? 0 : gbid.hashCode());
        result = prime * result + ((participantId == null) ? 0 : participantId.hashCode());
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
        GlobalDiscoveryEntryPersistedKey other = (GlobalDiscoveryEntryPersistedKey) obj;
        if (gbid == null) {
            if (other.gbid != null)
                return false;
        } else if (!gbid.equals(other.gbid))
            return false;
        if (participantId == null) {
            if (other.participantId != null)
                return false;
        } else if (!participantId.equals(other.participantId))
            return false;
        return true;
    }
}
