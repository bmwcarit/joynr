package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

import java.util.Collection;
import java.util.List;
import javax.annotation.CheckForNull;
import com.google.common.collect.Lists;
import joynr.system.RoutingTypes.ChannelAddress;
import joynr.types.CapabilityInformation;
import joynr.types.CommunicationMiddleware;
import joynr.types.DiscoveryEntry;

/**
 * Conversion helpers for CapabilityInformation, CapabilityEntry and DiscoveryEntry
 */
public class CapabilityUtils {

    public static DiscoveryEntry capabilityEntry2DiscoveryEntry(CapabilityEntry capabilityEntry) {
        return new DiscoveryEntry(capabilityEntry.getDomain(),
                                  capabilityEntry.getInterfaceName(),
                                  capabilityEntry.getParticipantId(),
                                  capabilityEntry.getProviderQos(),
                                  new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR });
    }

    @CheckForNull
    public static CapabilityInformation capabilityEntry2Information(CapabilityEntry capabilityEntry) {
        return capabilityEntry.toCapabilityInformation();
    }

    public static DiscoveryEntry capabilitiesInfo2DiscoveryEntry(CapabilityInformation capabilityInformation) {
        return new DiscoveryEntry(capabilityInformation.getDomain(),
                                  capabilityInformation.getInterfaceName(),
                                  capabilityInformation.getParticipantId(),
                                  capabilityInformation.getProviderQos(),
                                  new CommunicationMiddleware[]{ CommunicationMiddleware.JOYNR });
    }

    public static CapabilityEntry capabilitiesInfo2CapabilityEntry(CapabilityInformation capabilityInformation) {
        return new CapabilityEntryImpl(capabilityInformation.getDomain(),
                                       capabilityInformation.getInterfaceName(),
                                       capabilityInformation.getProviderQos(),
                                       capabilityInformation.getParticipantId(),
                                       System.currentTimeMillis(),
                                       new ChannelAddress(capabilityInformation.getChannelId()));
    }

    public static Collection<CapabilityEntry> capabilityInformationList2Entries(List<CapabilityInformation> capInfoList) {
        Collection<CapabilityEntry> capEntryCollection = Lists.newArrayList();
        for (CapabilityInformation capInfo : capInfoList) {
            capEntryCollection.add(capabilitiesInfo2CapabilityEntry(capInfo));
        }
        return capEntryCollection;
    }

    public static CapabilityInformation discoveryEntry2Information(DiscoveryEntry discoveryEntry, String channelId) {
        return new CapabilityInformation(discoveryEntry.getDomain(),
                                         discoveryEntry.getInterfaceName(),
                                         discoveryEntry.getQos(),
                                         channelId,
                                         discoveryEntry.getParticipantId());
    }

    public static CapabilityEntry discoveryEntry2CapEntry(DiscoveryEntry discoveryEntry, String channelId) {
        return new CapabilityEntryImpl(discoveryEntry.getDomain(),
                                       discoveryEntry.getInterfaceName(),
                                       discoveryEntry.getQos(),
                                       discoveryEntry.getParticipantId(),
                                       System.currentTimeMillis(),
                                       new ChannelAddress(channelId));
    }

    public static Collection<DiscoveryEntry> capabilityEntries2DiscoveryEntries(Collection<CapabilityEntry> capEntryList) {
        Collection<DiscoveryEntry> discoveryEntries = Lists.newArrayList();
        for (CapabilityEntry capabilityEntry : capEntryList) {
            discoveryEntries.add(capabilityEntry2DiscoveryEntry(capabilityEntry));
        }

        return discoveryEntries;
    }
}
