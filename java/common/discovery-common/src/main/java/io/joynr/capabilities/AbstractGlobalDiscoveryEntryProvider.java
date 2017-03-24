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

import com.google.inject.Provider;
import io.joynr.exceptions.JoynrRuntimeException;
import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

public abstract class AbstractGlobalDiscoveryEntryProvider implements Provider<GlobalDiscoveryEntry> {

    private final CapabilitiesProvisioning capabilitiesProvisioning;
    private final String interfaceName;

    public AbstractGlobalDiscoveryEntryProvider(CapabilitiesProvisioning capabilitiesProvisioning, String interfaceName) {
        assert capabilitiesProvisioning != null : "Capabilities provisioning must not be null.";
        assert interfaceName != null : "Interface name must not be null.";
        this.capabilitiesProvisioning = capabilitiesProvisioning;
        this.interfaceName = interfaceName;
    }

    @Override
    public GlobalDiscoveryEntry get() {
        GlobalDiscoveryEntry result = null;
        for (DiscoveryEntry discoveryEntry : capabilitiesProvisioning.getDiscoveryEntries()) {
            if (discoveryEntry instanceof GlobalDiscoveryEntry) {
                GlobalDiscoveryEntry globalDiscoveryEntry = (GlobalDiscoveryEntry) discoveryEntry;
                if (interfaceName.equals(globalDiscoveryEntry.getInterfaceName())) {
                    if (result != null) {
                        throw new JoynrRuntimeException("Duplicate discovery entry for " + interfaceName
                                + ". Make sure only one capability is provisioned for that interface.");
                    }
                    result = globalDiscoveryEntry;
                }
            }
        }
        if (result == null) {
            throw new JoynrRuntimeException("No discovery entry provisioned for capability " + interfaceName
                    + ". Exactly one must be provisioned.");
        }
        return result;
    }
}
