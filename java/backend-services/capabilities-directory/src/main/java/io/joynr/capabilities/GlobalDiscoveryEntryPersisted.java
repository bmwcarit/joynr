package io.joynr.capabilities;

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

import javax.persistence.Embedded;
import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.Table;

import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;

@Entity
@Table(name = "discovery_entries")
public class GlobalDiscoveryEntryPersisted extends GlobalDiscoveryEntry {
    private static final long serialVersionUID = 1L;

    public GlobalDiscoveryEntryPersisted() {
    }

    public GlobalDiscoveryEntryPersisted(GlobalDiscoveryEntry globalDiscoveryEntryObj) {
        super(globalDiscoveryEntryObj);
    }

    GlobalDiscoveryEntryPersisted(String domain,
                                  String interfaceName,
                                  String participantId,
                                  ProviderQos qos,
                                  long lastSeenDateMs,
                                  String address) {
        super(domain, interfaceName, participantId, qos, lastSeenDateMs, address);
    }

    @Override
    public String getDomain() {
        return super.getDomain();
    }

    @Override
    public String getInterfaceName() {
        return super.getInterfaceName();
    }

    @Override
    @Id
    public String getParticipantId() {
        return super.getParticipantId();
    }

    @Override
    @Embedded
    public ProviderQos getQos() {
        return super.getQos();
    }

    @Override
    public Long getLastSeenDateMs() {
        return super.getLastSeenDateMs();
    }

    @Override
    public String getAddress() {
        return super.getAddress();
    }
}
