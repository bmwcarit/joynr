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

import java.util.Collection;
import java.util.Set;

import javax.annotation.CheckForNull;

public interface CapabilitiesStore {

    public abstract void add(CapabilityEntry capabilityEntry);

    public abstract void add(Collection<? extends CapabilityEntry> interfaces);

    public abstract boolean remove(String participantId);

    public abstract void remove(Collection<String> participantIds);

    public abstract Collection<CapabilityEntry> lookup(String domain, String interfaceName, long cacheMaxAge);

    public abstract Collection<CapabilityEntry> lookup(String domain, String interfaceName);

    @CheckForNull
    public abstract CapabilityEntry lookup(String participantId, long cacheMaxAge);

    public abstract Set<CapabilityEntry> getAllCapabilities();

    public abstract boolean hasCapability(CapabilityEntry capabilityEntry);

}
