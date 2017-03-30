package io.joynr.capabilities;

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

import javax.persistence.Column;
import javax.persistence.Embedded;
import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.Lob;
import javax.persistence.Table;

import com.fasterxml.jackson.annotation.JsonProperty;

import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.Version;

@Entity
@Table(name = "discovery_entries")
public class GlobalDiscoveryEntryPersisted extends GlobalDiscoveryEntry {
    private static final long serialVersionUID = 1L;
    private ProviderQosPersisted providerQosPersisted;
    @JsonProperty("clusterControllerId")
    private String clusterControllerId;

    public GlobalDiscoveryEntryPersisted() {
    }

    public GlobalDiscoveryEntryPersisted(GlobalDiscoveryEntry globalDiscoveryEntryObj, String clusterControllerId) {
        super(globalDiscoveryEntryObj);
        this.clusterControllerId = clusterControllerId;
        providerQosPersisted = new ProviderQosPersisted(globalDiscoveryEntryObj.getQos());
    }

    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    GlobalDiscoveryEntryPersisted(Version providerVersion,
                                  String domain,
                                  String interfaceName,
                                  String participantId,
                                  ProviderQos qos,
                                  long lastSeenDateMs,
                                  long expiryDateMs,
                                  String publicKeyId,
                                  String address,
                                  String clusterControllerId) {
        // CHECKSTYLE ON
        super(providerVersion,
              domain,
              interfaceName,
              participantId,
              qos,
              lastSeenDateMs,
              expiryDateMs,
              publicKeyId,
              address);
        this.clusterControllerId = clusterControllerId;
        providerQosPersisted = new ProviderQosPersisted(qos);
    }

    @Override
    @Column
    public String getDomain() {
        return super.getDomain();
    }

    @Override
    @Column
    public String getInterfaceName() {
        return super.getInterfaceName();
    }

    @Override
    @Column
    @Id
    public String getParticipantId() {
        return super.getParticipantId();
    }

    @Column
    @Embedded
    public ProviderQosPersisted getProviderQosPersisted() {
        return providerQosPersisted;
    }

    public void setProviderQosPersisted(ProviderQosPersisted providerQosPersisted) {
        this.providerQosPersisted = providerQosPersisted;
        super.setQos(new ProviderQos(providerQosPersisted));
    }

    @Override
    @Column
    public Long getLastSeenDateMs() {
        return super.getLastSeenDateMs();
    }

    @Override
    @Column
    public Long getExpiryDateMs() {
        return super.getExpiryDateMs();
    }

    @Override
    @Column(length = 4000)
    @Lob
    public String getAddress() {
        return super.getAddress();
    }

    @Override
    @Column
    public String getPublicKeyId() {
        return super.getPublicKeyId();
    }

    @Column
    public Integer getMajorVersion() {
        return getProviderVersion().getMajorVersion();
    }

    public void setMajorVersion(Integer majorVersion) {
        getProviderVersion().setMajorVersion(majorVersion);
    }

    @Column
    public Integer getMinorVersion() {
        return getProviderVersion().getMinorVersion();
    }

    public void setMinorVersion(Integer minorVersion) {
        getProviderVersion().setMinorVersion(minorVersion);
    }

    @Column
    public String getClusterControllerId() {
        return clusterControllerId;
    }

    public void setClusterControllerId(String clusterControllerId) {
        this.clusterControllerId = clusterControllerId;
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj)
            return true;
        if (!super.equals(obj))
            return false;
        if (getClass() != obj.getClass())
            return false;
        GlobalDiscoveryEntryPersisted other = (GlobalDiscoveryEntryPersisted) obj;
        if (clusterControllerId == null) {
            if (other.clusterControllerId != null)
                return false;
        } else if (!clusterControllerId.equals(other.clusterControllerId))
            return false;
        return true;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = super.hashCode();
        result = prime * result + ((clusterControllerId == null) ? 0 : clusterControllerId.hashCode());
        return result;
    }
}
