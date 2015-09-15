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

import io.joynr.dispatcher.rpc.JoynrInterface;
import io.joynr.endpoints.AddressPersisted;
import io.joynr.endpoints.JoynrMessagingEndpointAddressPersisted;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import javax.annotation.CheckForNull;
import javax.persistence.CascadeType;
import javax.persistence.DiscriminatorColumn;
import javax.persistence.DiscriminatorType;
import javax.persistence.DiscriminatorValue;
import javax.persistence.Entity;
import javax.persistence.Id;
import javax.persistence.Inheritance;
import javax.persistence.JoinColumn;
import javax.persistence.OneToMany;
import javax.persistence.OneToOne;
import javax.persistence.Table;

import joynr.system.routingtypes.Address;
import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@Entity
@Inheritance
@DiscriminatorColumn(name = "cached", discriminatorType = DiscriminatorType.CHAR)
@DiscriminatorValue("N")
@Table(name = "capabilities")
public class CapabilityEntryPersisted implements CapabilityEntry, Serializable {
    private static final long serialVersionUID = 1L;

    private static final Logger logger = LoggerFactory.getLogger(CapabilityEntryPersisted.class);

    @Id
    protected String participantId;

    protected String domain;
    protected String interfaceName;

    @OneToOne(cascade = CascadeType.ALL)
    @JoinColumn(name = "participantId", referencedColumnName = "participantId")
    protected ProviderQosPersisted providerQos;

    @OneToMany(cascade = CascadeType.ALL, targetEntity = AddressPersisted.class)
    @JoinColumn(name = "participantId", referencedColumnName = "participantId")
    protected List<AddressPersisted> endpointAddresses;

    protected long dateWhenRegistered;
    protected Origin origin;

    public CapabilityEntryPersisted() {
        origin = Origin.LOCAL;
    }

    public CapabilityEntryPersisted(String domain,
                                    String interfaceName,
                                    ProviderQos providerQos,
                                    String participantId,
                                    long dateWhenRegistered,
                                    AddressPersisted... endpointAddresses) {

        this.domain = domain;
        this.interfaceName = interfaceName;
        this.providerQos = new ProviderQosPersisted(participantId, providerQos);
        this.participantId = participantId;
        this.dateWhenRegistered = dateWhenRegistered;
        origin = Origin.LOCAL;
        this.endpointAddresses = new ArrayList<AddressPersisted>(Arrays.asList(endpointAddresses));
    }

    public <T extends JoynrInterface> CapabilityEntryPersisted(String domain,
                                                               Class<T> providedInterface,
                                                               ProviderQos providerQos,
                                                               String participantId,
                                                               long dateWhenRegistered,
                                                               AddressPersisted... endpointAddresses) {
        this(domain, "", providerQos, participantId, dateWhenRegistered, endpointAddresses);
        String name = null;
        String reason = "shadow field INTERFACE_NAME in your interface";
        try {
            name = (String) providedInterface.getField("INTERFACE_NAME").get(String.class);
        } catch (Exception e) {
            reason = reason + ": " + e.getMessage();
        }

        if (name == null) {
            logger.error("INTERFACE_NAME not set in class {}", providedInterface.getSimpleName());
            throw new IllegalArgumentException(reason);
        }
        this.interfaceName = name;
    }

    public CapabilityEntryPersisted(CapabilityInformation capabilityInformation) {
        this(capabilityInformation.getDomain(),
             capabilityInformation.getInterfaceName(),
             capabilityInformation.getProviderQos(),
             capabilityInformation.getParticipantId(),
             System.currentTimeMillis(),
             new JoynrMessagingEndpointAddressPersisted(capabilityInformation.getChannelId()));
    }

    public static CapabilityEntryPersisted fromCapabilityInformation(CapabilityInformation capInfo) {
        return new CapabilityEntryPersisted(capInfo.getDomain(),
                                            capInfo.getInterfaceName(),
                                            capInfo.getProviderQos(),
                                            capInfo.getParticipantId(),
                                            System.currentTimeMillis(),
                                            // Assume the Capability entry is not local because it has been serialized
                                            new JoynrMessagingEndpointAddressPersisted(capInfo.getChannelId()));
    }

    @Override
    @CheckForNull
    public CapabilityInformation toCapabilityInformation() {
        String channelId = null;
        for (Address endpointAddress : getAddresses()) {
            if (endpointAddress instanceof JoynrMessagingEndpointAddressPersisted) {
                channelId = ((JoynrMessagingEndpointAddressPersisted) endpointAddress).getChannelId();
                break;
            }
        }
        if (channelId == null) {
            return null;
        }
        return new CapabilityInformation(getDomain(),
                                         getInterfaceName(),
                                         new ProviderQos(getProviderQos()),
                                         channelId,
                                         getParticipantId());
    }

    @Override
    public ProviderQos getProviderQos() {
        return providerQos;
    }

    @Override
    public List<Address> getAddresses() {
        return Collections.<Address> unmodifiableList(endpointAddresses);
    }

    @Override
    public String getParticipantId() {
        return participantId;
    }

    @Override
    public String getDomain() {
        return domain;
    }

    @Override
    public String getInterfaceName() {
        return interfaceName;
    }

    @Override
    public long getDateWhenRegistered() {
        return dateWhenRegistered;
    }

    @Override
    public void setDateWhenRegistered(long dateWhenRegistered) {
        this.dateWhenRegistered = dateWhenRegistered;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + (int) (dateWhenRegistered ^ (dateWhenRegistered >>> 32));
        result = prime * result + ((domain == null) ? 0 : domain.hashCode());
        result = prime * result + ((endpointAddresses == null) ? 0 : endpointAddresses.hashCode());
        result = prime * result + ((interfaceName == null) ? 0 : interfaceName.hashCode());
        result = prime * result + ((origin == null) ? 0 : origin.hashCode());
        result = prime * result + ((participantId == null) ? 0 : participantId.hashCode());
        result = prime * result + ((providerQos == null) ? 0 : providerQos.hashCode());
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
        CapabilityEntryPersisted other = (CapabilityEntryPersisted) obj;
        if (dateWhenRegistered != other.dateWhenRegistered) {
            return false;
        }
        if (domain == null) {
            if (other.domain != null) {
                return false;
            }
        } else if (!domain.equals(other.domain)) {
            return false;
        }
        if (endpointAddresses == null) {
            if (other.endpointAddresses != null) {
                return false;
            }
        } else if (!endpointAddresses.equals(other.endpointAddresses)) {
            return false;
        }
        if (interfaceName == null) {
            if (other.interfaceName != null) {
                return false;
            }
        } else if (!interfaceName.equals(other.interfaceName)) {
            return false;
        }
        if (origin != other.origin) {
            return false;
        }
        if (participantId == null) {
            if (other.participantId != null) {
                return false;
            }
        } else if (!participantId.equals(other.participantId)) {
            return false;
        }
        if (providerQos == null) {
            if (other.providerQos != null) {
                return false;
            }
        } else if (!providerQos.equals(other.providerQos)) {
            return false;
        }
        return true;
    }

    @Override
    public void setOrigin(Origin remote) {
        // TODO Auto-generated method stub

    }

    @Override
    public void addEndpoint(Address endpointAddress) {
        if (endpointAddress instanceof AddressPersisted) {
            endpointAddresses.add((AddressPersisted) endpointAddress);
        }
    }
}
