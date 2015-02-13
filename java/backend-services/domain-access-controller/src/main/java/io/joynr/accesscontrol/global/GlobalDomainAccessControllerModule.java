package io.joynr.accesscontrol.global;

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

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.name.Named;
import io.joynr.accesscontrol.DomainAccessControlStore;
import io.joynr.accesscontrol.DomainAccessControlStoreEhCache;
import io.joynr.messaging.ConfigurableMessagingSettings;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.infrastructure.GlobalDomainAccessControllerAbstractProvider;
import net.sf.ehcache.CacheManager;
import net.sf.ehcache.config.Configuration;

public class GlobalDomainAccessControllerModule extends AbstractModule {

    @Override
    protected void configure() {

        bind(GlobalDomainAccessControllerAbstractProvider.class).to(GlobalDomainAccessControllerProviderImpl.class);
        bind(DomainAccessControlStore.class).to(DomainAccessControlStoreEhCache.class);
    }

    @Provides
    @Named(AbstractJoynrApplication.PROPERTY_JOYNR_DOMAIN_LOCAL)
    String provideAccessControlDomain(@Named(ConfigurableMessagingSettings.PROPERTY_DISCOVERY_DIRECTORIES_DOMAIN) String aclDomain) {
        return aclDomain;
    }

    @Provides
    @Named(MessagingPropertyKeys.CHANNELID)
    String provideAccessControlChannelId(@Named(ConfigurableMessagingSettings.PROPERTY_DOMAIN_ACCESS_CONTROLLER_CHANNEL_ID) String accessControlChannelId) {
        return accessControlChannelId;
    }

    @Provides
    CacheManager provideCacheManager() {
        Configuration configuration = new Configuration();
        configuration.setName("GDACEhCacheManager");
        return CacheManager.create(configuration);
    }
}
