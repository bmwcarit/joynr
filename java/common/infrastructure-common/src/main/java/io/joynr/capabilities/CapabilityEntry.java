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
import io.joynr.endpoints.EndpointAddressBase;
import io.joynr.endpoints.JoynrMessagingEndpointAddress;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import javax.annotation.CheckForNull;

import joynr.types.CapabilityInformation;
import joynr.types.ProviderQos;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

@SuppressWarnings("serial")
// used as super-type token to present type erasure from generics
class EndpointList extends ArrayList<EndpointAddressBase> {
    public EndpointList() {
        super();
    }

    public EndpointList(EndpointAddressBase initialEntry) {
        this();
        this.add(initialEntry);
    }
}

public class CapabilityEntry implements Comparable<CapabilityEntry>, Serializable {
    /**
     * 
     */
    private static final long serialVersionUID = 1L;

    private static final Logger logger = LoggerFactory.getLogger(CapabilityEntry.class);

    protected ProviderQos providerQos;
    protected EndpointList endpointAddresses = new EndpointList();
    protected String participantId;
    protected String domain;
    protected String interfaceName;

    public CapabilityEntry() {

    }

    public CapabilityEntry(String domain,
                           String interfaceName,
                           ProviderQos providerQos,
                           EndpointAddressBase endpointAddress,
                           String participantId) {
        this.interfaceName = interfaceName;

        this.providerQos = providerQos;
        this.endpointAddresses.add(endpointAddress);
        this.participantId = participantId;
        this.domain = domain;
    }

    public CapabilityEntry(String domain,
                           String interfaceName,
                           ProviderQos providerQos,
                           List<EndpointAddressBase> endpointAddresses,
                           String participantId) {
        this.interfaceName = interfaceName;

        this.providerQos = providerQos;
        this.endpointAddresses.addAll(endpointAddresses);
        this.participantId = participantId;
        this.domain = domain;
    }

    public CapabilityEntry(String domain, String interfaceName, ProviderQos providerQos, String participantId) {
        this(domain, interfaceName, providerQos, new EndpointList(), participantId);
    }

    public <T extends JoynrInterface> CapabilityEntry(String domain,
                                                      Class<T> providedInterface,
                                                      ProviderQos providerQos,
                                                      List<EndpointAddressBase> endpointAddressList,
                                                      String participantId) {
        this(domain, "", providerQos, endpointAddressList, participantId);
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

    public <T extends JoynrInterface> CapabilityEntry(String domain,
                                                      Class<T> providedInterface,
                                                      ProviderQos providerQos,
                                                      EndpointAddressBase endpointAddress,
                                                      String participantId) {
        this(domain, providedInterface, providerQos, new EndpointList(endpointAddress), participantId);
    }

    public <T extends JoynrInterface> CapabilityEntry(String domain,
                                                      Class<T> providedInterface,
                                                      ProviderQos providerQos,
                                                      String participantId) {
        this(domain, providedInterface, providerQos, new EndpointList(), participantId);

    }

    public static CapabilityEntry fromCapabilityInformation(CapabilityInformation capInfo) {
        return new CapabilityEntry(capInfo.getDomain(),
                                   capInfo.getInterfaceName(),
                                   capInfo.getProviderQos(),
                                   new JoynrMessagingEndpointAddress(capInfo.getChannelId()),
                                   capInfo.getParticipantId()); // Assume the Capability entry is not local because it has been serialized
    }

    @CheckForNull
    public CapabilityInformation toCapabilityInformation() {
        String channelId = null;
        for (EndpointAddressBase endpointAddress : getEndpointAddresses()) {
            if (endpointAddress instanceof JoynrMessagingEndpointAddress) {
                channelId = ((JoynrMessagingEndpointAddress) endpointAddress).getChannelId();
                break;
            }
        }
        if (channelId == null) {
            return null;
        }
        return new CapabilityInformation(getDomain(),
                                         getInterfaceName(),
                                         getProviderQos(),
                                         channelId,
                                         getParticipantId());
    }

    public ProviderQos getProviderQos() {
        return providerQos;
    }

    public List<EndpointAddressBase> getEndpointAddresses() {
        return endpointAddresses;
    }

    public String getParticipantId() {
        return participantId;
    }

    public String getDomain() {
        return domain;
    }

    public String getInterfaceName() {
        return interfaceName;
    }

    @Override
    public int hashCode() {
        final int prime = 31;
        int result = 1;
        result = prime * result + ((domain == null) ? 0 : domain.hashCode());
        result = prime * result + ((endpointAddresses == null) ? 0 : endpointAddresses.hashCode());
        result = prime * result + ((interfaceName == null) ? 0 : interfaceName.hashCode());
        result = prime * result + ((participantId == null) ? 0 : participantId.hashCode());
        result = prime * result + ((providerQos == null) ? 0 : providerQos.hashCode());
        return result;
    }

    @Override
    public String toString() {
        StringBuilder stringBuilder = new StringBuilder();
        stringBuilder.append("domain: ");
        stringBuilder.append(domain);
        stringBuilder.append("\r\n");
        stringBuilder.append("interface: ");
        stringBuilder.append(interfaceName);
        stringBuilder.append("\r\n");
        stringBuilder.append("participant: ");
        stringBuilder.append(participantId);
        stringBuilder.append("\r\n");
        stringBuilder.append("endpoint: ");
        stringBuilder.append(endpointAddresses);
        stringBuilder.append("\r\n");
        return stringBuilder.toString();

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
        CapabilityEntry other = (CapabilityEntry) obj;
        if (domain == null) {
            if (other.domain != null) {
                return false;
            }
        } else if (!domain.equals(other.domain)) {
            return false;
        }
        // TODO removed to enable CapabilitiesStore.removeCapability
        // if (endpointAddresses == null) {
        // if (other.endpointAddresses != null) {
        // return false;
        // }
        // } else if (!endpointAddresses.equals(other.endpointAddresses)) {
        // return false;
        // }
        if (interfaceName == null) {
            if (other.interfaceName != null) {
                return false;
            }
        } else if (!interfaceName.equals(other.interfaceName)) {
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

    public void addEndpoint(EndpointAddressBase endpointAddress) {
        endpointAddresses.add(endpointAddress);

    }

    @Override
    public int compareTo(CapabilityEntry o) {
        // TODO Auto-generated method stub
        return 0;
    }

}
