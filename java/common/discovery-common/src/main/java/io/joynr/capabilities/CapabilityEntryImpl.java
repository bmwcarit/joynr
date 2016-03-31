package io.joynr.capabilities;

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

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import javax.annotation.CheckForNull;

import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;
import joynr.types.Version;

public class CapabilityEntryImpl implements CapabilityEntry {

    protected Version providerVersion;
    protected String participantId;
    protected String domain;
    protected String interfaceName;

    protected ProviderQos providerQos;

    protected List<Address> addresses;

    protected long dateWhenRegistered;
    protected Origin origin;

    public CapabilityEntryImpl() {
        origin = Origin.LOCAL;
    }

    public CapabilityEntryImpl(Version providerVersion,
                               String domain,
                               String interfaceName,
                               ProviderQos providerQos,
                               String participantId,
                               long dateWhenRegistered,
                               Address... addresses) {

        this.providerVersion = providerVersion;
        this.domain = domain;
        this.interfaceName = interfaceName;
        this.providerQos = providerQos;
        this.participantId = participantId;
        this.dateWhenRegistered = dateWhenRegistered;
        origin = Origin.LOCAL;
        this.addresses = new ArrayList<Address>();
        this.addresses.addAll(Arrays.asList(addresses));
    }

    public CapabilityEntryImpl(CapabilityInformation capInfo) {
        this(capInfo.getProviderVersion(),
             capInfo.getDomain(),
             capInfo.getInterfaceName(),
             capInfo.getProviderQos(),
             capInfo.getParticipantId(),
             System.currentTimeMillis(),
             // Assume the Capability entry is not local because it has been serialized
             new ChannelAddress(capInfo.getChannelId()));
    }

    public static CapabilityEntryImpl fromCapabilityInformation(CapabilityInformation capInfo) {
        return new CapabilityEntryImpl(capInfo.getProviderVersion(),
                                       capInfo.getDomain(),
                                       capInfo.getInterfaceName(),
                                       capInfo.getProviderQos(),
                                       capInfo.getParticipantId(),
                                       System.currentTimeMillis(),
                                       // Assume the Capability entry is not local because it has been serialized
                                       new ChannelAddress(capInfo.getChannelId()));
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#toCapabilityInformation()
     */
    @Override
    @CheckForNull
    public CapabilityInformation toCapabilityInformation() {
        String channelId = null;
        for (Address endpointAddress : getAddresses()) {
            if (endpointAddress instanceof ChannelAddress) {
                channelId = ((ChannelAddress) endpointAddress).getChannelId();
                break;
            }
        }
        if (channelId == null) {
            return null;
        }
        return new CapabilityInformation(getProviderVersion(),
                                         getDomain(),
                                         getInterfaceName(),
                                         new ProviderQos(getProviderQos()),
                                         channelId,
                                         getParticipantId());
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getProviderQos()
     */
    @Override
    public Version getProviderVersion() {
        return providerVersion;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getProviderQos()
     */
    @Override
    public ProviderQos getProviderQos() {
        return providerQos;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getEndpointAddresses()
     */
    @Override
    public List<Address> getAddresses() {
        return addresses;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getParticipantId()
     */
    @Override
    public String getParticipantId() {
        return participantId;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getDomain()
     */
    @Override
    public String getDomain() {
        return domain;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getInterfaceName()
     */
    @Override
    public String getInterfaceName() {
        return interfaceName;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#getDateWhenRegistered()
     */
    @Override
    public long getDateWhenRegistered() {
        return dateWhenRegistered;
    }

    protected Origin getOrigin() {
        return origin;
    }

    /* (non-Javadoc)
     * @see io.joynr.capabilities.CapabilityEntry#setDateWhenRegistered(long)
     */
    @Override
    public void setDateWhenRegistered(long dateWhenRegistered) {
        this.dateWhenRegistered = dateWhenRegistered;
    }

    @Override
    public void setOrigin(Origin origin) {
        this.origin = origin;
    }

    protected void setVersion(Version providerVersion) {
        this.providerVersion = providerVersion;
    }

    protected void setProviderQos(ProviderQos providerQos) {
        this.providerQos = providerQos;
    }

    protected final void setEndpointAddresses(List<Address> addresses) {
        this.addresses = addresses;
    }

    protected final void setParticipantId(String participantId) {
        this.participantId = participantId;
    }

    protected final void setDomain(String domain) {
        this.domain = domain;
    }

    protected final void setInterfaceName(String interfaceName) {
        this.interfaceName = interfaceName;
    }

    @Override
    public String toString() {
        StringBuilder builder = new StringBuilder();
        builder.append("CapabilityEntry [providerQos=")
               .append(providerQos)
               .append(", addresses=")
               .append(addresses)
               .append(", participantId=")
               .append(participantId)
               .append(", domain=")
               .append(domain)
               .append(", interfaceName=")
               .append(interfaceName)
               .append(", dateWhenRegistered=")
               .append(getDateWhenRegistered())
               .append(", providerVersion=")
               .append(getProviderVersion())
               .append("]");
        return builder.toString();
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (getDateWhenRegistered() ^ (getDateWhenRegistered() >>> 32));
        result = prime * result + ((domain == null) ? 0 : domain.hashCode());
        result = prime * result + ((addresses == null) ? 0 : addresses.hashCode());
        result = prime * result + ((interfaceName == null) ? 0 : interfaceName.hashCode());
        result = prime * result + ((participantId == null) ? 0 : participantId.hashCode());
        result = prime * result + ((providerQos == null) ? 0 : providerQos.hashCode());
        result = prime * result + ((providerVersion == null) ? 0 : providerVersion.hashCode());
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
        CapabilityEntryImpl other = (CapabilityEntryImpl) obj;
        if (participantId == null) {
            if (other.participantId != null) {
                return false;
            }
        } else if (!participantId.equals(other.participantId)) {
            return false;
        }
        if (domain == null) {
            if (other.domain != null) {
                return false;
            }
        } else if (!domain.equals(other.domain)) {
            return false;
        }
        if (addresses == null) {
            if (other.addresses != null) {
                return false;
            }
        } else if (!addresses.equals(other.addresses)) {
            return false;
        }
        if (interfaceName == null) {
            if (other.interfaceName != null) {
                return false;
            }
        } else if (!interfaceName.equals(other.interfaceName)) {
            return false;
        }
        if (providerQos == null) {
            if (other.providerQos != null) {
                return false;
            }
        } else if (!providerQos.equals(other.providerQos)) {
            return false;
        }
        if (providerVersion == null) {
            if (other.providerVersion != null) {
                return false;
            }
        } else if (!providerVersion.equals(other.providerVersion)) {
            return false;
        }
        return true;
    }

    @Override
    public void addEndpoint(Address endpointAddress) {
        if (!addresses.contains(endpointAddress)) {
            addresses.add(endpointAddress);
        }
    }
}
