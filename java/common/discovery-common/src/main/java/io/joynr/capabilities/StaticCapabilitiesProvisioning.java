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

import static java.lang.String.format;

import java.io.IOException;

import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.core.type.TypeReference;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.exceptions.JoynrRuntimeException;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

/**
 * Loads a set of JSON encoded {@link GlobalDiscoveryEntry discovery entries} from the property named
 * {@link #PROPERTY_PROVISIONED_CAPABILITIES joynr.capabilities.provisioned} and makes them available via
 * {@link #getDiscoveryEntries()}.
 *
 * This component will fail-fast - that is, it will throw a {@link JoynrRuntimeException} during initialization if the
 * JSON read from the property cannot bit parsed.
 */
public class StaticCapabilitiesProvisioning implements CapabilitiesProvisioning {

    public static final String PROPERTY_PROVISIONED_CAPABILITIES = "joynr.capabilities.provisioned";
    private Collection<DiscoveryEntry> discoveryEntries;
    private static Logger logger = LoggerFactory.getLogger(StaticCapabilitiesProvisioning.class);

    @Inject
    public StaticCapabilitiesProvisioning(@Named(PROPERTY_PROVISIONED_CAPABILITIES) String provisionedCapabilitiesJsonString,
                                          ObjectMapper objectMapper) {
        loadDiscoveryEntries(provisionedCapabilitiesJsonString, objectMapper);
    }

    private void loadDiscoveryEntries(String provisionedCapabilitiesJsonString, ObjectMapper objectMapper) {
        discoveryEntries = new HashSet<DiscoveryEntry>();
        logger.debug("Statically provisioned capabilities properties value: {}", provisionedCapabilitiesJsonString);
        List<GlobalDiscoveryEntry> newEntries = null;
        try {
            newEntries = objectMapper.readValue(provisionedCapabilitiesJsonString,
                                                new TypeReference<List<GlobalDiscoveryEntry>>() {
                                                });
            logger.debug("Statically provisioned entries loaded: {}", newEntries);
            for (GlobalDiscoveryEntry globalDiscoveryEntry : newEntries) {
                discoveryEntries.add(globalDiscoveryEntry);
            }
        } catch (IOException e) {
            String message = format("Unable to load provisioned capabilities. Invalid JSON value: %s",
                                    provisionedCapabilitiesJsonString);
            throw new JoynrRuntimeException(message, e);
        }
    }

    @Override
    public Collection<DiscoveryEntry> getDiscoveryEntries() {
        return discoveryEntries;
    }
}
