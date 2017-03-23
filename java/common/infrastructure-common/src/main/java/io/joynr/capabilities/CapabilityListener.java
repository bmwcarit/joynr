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

import joynr.types.DiscoveryEntry;

/**
 * Listener for changes in the capability directory.
 */
public interface CapabilityListener {
    /**
     * Called when a capability entry was added to the capabilities directory.
     * @param addedCapability the entry that was added.
     */
    void capabilityAdded(DiscoveryEntry addedCapability);

    /**
     * Called when a capability entry was removed from the capabilities
     * directory.
     * @param removedCapability the entry that was removed.
     */
    void capabilityRemoved(DiscoveryEntry removedCapability);
}
