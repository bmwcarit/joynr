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

import java.io.IOException;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Properties;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

public class StaticCapabilitiesProvisioning implements CapabilitiesProvisioning {
    public static final String STATIC_PROVISIONING_PROPERTIES = "static_capabilities_provisioning.properties";
    public static final String PROPERTY_PROVISIONED_CAPABILITIES = "joynr.capabilities.provisioned";
    private Collection<DiscoveryEntry> discoveryEntries;
    private static Logger logger = LoggerFactory.getLogger(StaticCapabilitiesProvisioning.class);

    @Inject
    public StaticCapabilitiesProvisioning(@Named(STATIC_PROVISIONING_PROPERTIES) Properties properties,
                                          ObjectMapper objectMapper) {
        loadDiscoveryEntries(properties, objectMapper);
    }

    private void loadDiscoveryEntries(Properties properties, ObjectMapper objectMapper) {
        discoveryEntries = new HashSet<DiscoveryEntry>();
        String entries = properties.getProperty(PROPERTY_PROVISIONED_CAPABILITIES);
        logger.debug("Statically provisioned capabilities properties value: {}", entries);
        List<GlobalDiscoveryEntry> newEntries = null;
        try {
            newEntries = objectMapper.readValue(entries, new TypeReference<List<GlobalDiscoveryEntry>>() {
            });
            logger.debug("Statically provisioned entries loaded: {}", newEntries);
            for (GlobalDiscoveryEntry globalDiscoveryEntry : newEntries) {
                discoveryEntries.add(globalDiscoveryEntry);
            }
        } catch (IOException e) {
            logger.error("Unable to load provisioned capabilities. Invalid JSON value: {}", entries, e);
        }
    }

    @Override
    public Collection<DiscoveryEntry> getDiscoveryEntries() {
        return discoveryEntries;
    }
}
