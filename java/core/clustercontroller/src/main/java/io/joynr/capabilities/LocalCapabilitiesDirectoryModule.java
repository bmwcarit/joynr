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
package io.joynr.capabilities;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;

import joynr.types.DiscoveryEntry;
import joynr.types.GlobalDiscoveryEntry;

public class LocalCapabilitiesDirectoryModule extends AbstractModule {
    @Provides
    DiscoveryEntryStore<DiscoveryEntry> providesLocalStore() {
        return new DiscoveryEntryStoreInMemory<DiscoveryEntry>(0);
    }

    @Provides
    DiscoveryEntryStore<GlobalDiscoveryEntry> providesGlobalCache() {
        return new DiscoveryEntryStoreInMemory<GlobalDiscoveryEntry>(1000);
    }

    @Override
    protected void configure() {
        bind(LocalCapabilitiesDirectory.class).to(LocalCapabilitiesDirectoryImpl.class).in(Singleton.class);
    }
}
