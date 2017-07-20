package io.joynr.arbitration;

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

import java.util.HashSet;
import java.util.Set;

import joynr.types.DiscoveryEntryWithMetaInfo;

public class ArbitrationResult {
    private Set<DiscoveryEntryWithMetaInfo> discoveryEntries;

    public ArbitrationResult(final DiscoveryEntryWithMetaInfo... discoveryEntries) {
        this.discoveryEntries = new HashSet<>();
        if (discoveryEntries != null && discoveryEntries.length > 0) {
            for (DiscoveryEntryWithMetaInfo discoveryEntry : discoveryEntries) {
                this.discoveryEntries.add(discoveryEntry);
            }
        }
    }

    public ArbitrationResult() {
    }

    public Set<DiscoveryEntryWithMetaInfo> getDiscoveryEntries() {
        return discoveryEntries;
    }

    public void setDiscoveryEntries(Set<DiscoveryEntryWithMetaInfo> discoveryEntries) {
        this.discoveryEntries = discoveryEntries;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((discoveryEntries == null) ? 0 : discoveryEntries.hashCode());
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
        if (discoveryEntries == null) {
            if (other.discoveryEntries != null) {
                return false;
            }
        } else if (!discoveryEntries.equals(other.discoveryEntries)) {
            return false;
        }
        return true;
    }

}
