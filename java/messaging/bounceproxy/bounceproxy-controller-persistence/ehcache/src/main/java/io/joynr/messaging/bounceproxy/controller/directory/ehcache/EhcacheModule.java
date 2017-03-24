package io.joynr.messaging.bounceproxy.controller.directory.ehcache;

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

import java.net.URL;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import net.sf.ehcache.CacheManager;
import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.inmemory.InMemoryChannelDirectory;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

public class EhcacheModule extends AbstractModule {

    private final static Logger log = LoggerFactory.getLogger(EhcacheModule.class);

    @Override
    protected void configure() {
        bind(BounceProxyDirectory.class).to(BounceProxyEhcacheAdapter.class).asEagerSingleton();
        bind(ChannelDirectory.class).to(InMemoryChannelDirectory.class);
    }

    @Provides
    @Singleton
    CacheManager getCacheManager(@Named(BounceProxyEhcacheAdapter.PROPERTY_BP_CACHE_CONFIGURATION) String cacheConfigFileName) {

        log.info("Using ehcache config file: {}", cacheConfigFileName);
        URL cacheConfigFileUrl = getClass().getResource("/" + cacheConfigFileName);
        if (cacheConfigFileUrl == null) {
            log.error("No resource with filename found on classpath: {}. Using default CacheManager",
                      cacheConfigFileName);
            return CacheManager.newInstance();
        } else {
            return CacheManager.newInstance(cacheConfigFileUrl);
        }
    }

}
