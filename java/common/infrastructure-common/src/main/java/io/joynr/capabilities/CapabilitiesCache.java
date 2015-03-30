package io.joynr.capabilities;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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

import io.joynr.capabilities.CapabilityEntry.Origin;

import com.google.inject.Inject;

/**
 * The CapabilitiesCache is used as a cache for capabilities retried from a remote directory
 */
public class CapabilitiesCache extends CapabilitiesStoreImpl {

    @Inject
    public CapabilitiesCache(CapabilitiesProvisioning staticProvisioning) {
        super(staticProvisioning);
    }

    @Override
    public void add(CapabilityEntry capabilityEntry) {
        capabilityEntry.setOrigin(Origin.REMOTE);
        super.add(capabilityEntry);
    }

}
