package io.joynr.capabilities;

/*
 * #%L
 * joynr::java::common::infrastructurecommon
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

import io.joynr.endpoints.JoynrMessagingEndpointAddress;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Properties;

import joynr.types.CapabilityInformation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

public class StaticCapabilitiesProvisioning implements CapabilitiesProvisioning {
    public static final String STATIC_PROVISIONING_PROPERTIES = "static_capabilities_provisioning.properties";
    private static final String provisioningEntry = "provisionedCapabilities";
    private List<CapabilityEntry> capabilityEntries;
    private static Logger logger = LoggerFactory.getLogger(StaticCapabilitiesProvisioning.class);

    @Inject
    public StaticCapabilitiesProvisioning(@Named(STATIC_PROVISIONING_PROPERTIES) Properties properties,
                                          ObjectMapper objectMapper) {
        loadCapabilityEntries(properties, objectMapper);
    }

    @SuppressWarnings("unchecked")
    private void loadCapabilityEntries(Properties properties, ObjectMapper objectMapper) {
        capabilityEntries = new ArrayList<CapabilityEntry>();
        Object entries = properties.get(provisioningEntry);
        Object newEntries = null;
        try {
            newEntries = objectMapper.readValue((String) entries, new TypeReference<List<CapabilityInformation>>() {
            });
            List<CapabilityInformation> castedEntries = (List<CapabilityInformation>) newEntries;
            for (CapabilityInformation capabilityInformation : castedEntries) {
                capabilityEntries.add(new CapabilityEntry(capabilityInformation.getDomain(),
                                                          capabilityInformation.getInterfaceName(),
                                                          capabilityInformation.getProviderQos(),
                                                          new JoynrMessagingEndpointAddress(capabilityInformation.getChannelId()),
                                                          capabilityInformation.getParticipantId(),
                                                          CapabilityScope.REMOTE));
            }
        } catch (Exception e) {
            logger.error("unable to load provisioned capabilities. "
                    + (newEntries != null ? "to be processed entry: " + newEntries : ""), e);
        }
    }

    @Override
    public Collection<? extends CapabilityEntry> getCapabilityEntries() {
        return capabilityEntries;
    }
}
