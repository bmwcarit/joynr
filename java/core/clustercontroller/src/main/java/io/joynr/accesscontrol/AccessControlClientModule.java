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
package io.joynr.accesscontrol;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;

import net.sf.ehcache.CacheManager;
import net.sf.ehcache.config.Configuration;

public class AccessControlClientModule extends AbstractModule {
    @Override
    protected void configure() {
        bind(AccessController.class).to(AccessControllerImpl.class).in(Singleton.class);
        bind(LocalDomainAccessController.class).to(LocalDomainAccessControllerImpl.class).in(Singleton.class);
        bind(DomainAccessControlStore.class).to(DomainAccessControlStoreCqEngine.class).in(Singleton.class);
        bind(DomainAccessControlProvisioning.class).to(StaticDomainAccessControlProvisioning.class);
    }

    @Provides
    @Singleton
    public CacheManager provideCacheManager() {
        Configuration configuration = new Configuration();
        configuration.setName("LDACEhCacheManager");
        configuration.setUpdateCheck(false);
        return CacheManager.create(configuration);
    }
}
