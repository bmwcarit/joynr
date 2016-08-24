package io.joynr.arbitration;

import java.util.HashSet;
import java.util.Set;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

public class ArbitrationResult {
    private Set<String> participantIds;

    public ArbitrationResult(final String ... participantIds) {
        this.participantIds = new HashSet<>();
        if (participantIds != null && participantIds.length > 0) {
			for (String participantId : participantIds) {
				this.participantIds.add(participantId);
			}
        }
    }

    public ArbitrationResult() {
    }

    public Set<String> getParticipantIds() {
        return participantIds;
    }

    public void setParticipantIds(Set<String> participantIds) {
        this.participantIds = participantIds;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((participantIds == null) ? 0 : participantIds.hashCode());
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
        if (participantIds == null) {
            if (other.participantIds != null) {
                return false;
            }
        } else if (!participantIds.equals(other.participantIds)) {
            return false;
        }
        return true;
    }

}
