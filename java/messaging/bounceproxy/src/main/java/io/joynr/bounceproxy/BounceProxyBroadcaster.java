package io.joynr.bounceproxy;

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

import io.joynr.bounceproxy.filter.ExpirationFilter;
import io.joynr.bounceproxy.filter.MessageSerializationFilter;

import java.util.Set;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import org.atmosphere.cache.UUIDBroadcasterCache;
import org.atmosphere.cpr.AtmosphereConfig;
import org.atmosphere.cpr.AtmosphereResource;
import org.atmosphere.util.SimpleBroadcaster;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class BounceProxyBroadcaster extends SimpleBroadcaster {
    private static final Logger logger = LoggerFactory.getLogger(BounceProxyBroadcaster.class);

    public BounceProxyBroadcaster(String name, AtmosphereConfig config) {
        super(name, config);

        UUIDBroadcasterCache broadcasterCache = (UUIDBroadcasterCache) bc.getBroadcasterCache();
        broadcasterCache.setClientIdleTime(TimeUnit.DAYS.toMillis(7));

        // order of filters matters
        bc.addFilter(new ExpirationFilter());
        bc.addFilter(new MessageSerializationFilter());

    }

    @Override
    public synchronized void cacheLostMessage(AtmosphereResource r) {
        logger.trace("cacheLostMessage 1");
        super.cacheLostMessage(r);
    }

    @Override
    public synchronized void cacheLostMessage(AtmosphereResource r, AsyncWriteToken token) {
        logger.trace("cacheLostMessage 2");
        super.cacheLostMessage(r, token);
    }

    @Override
    public synchronized void cacheLostMessage(AtmosphereResource r, AsyncWriteToken token, boolean force) {
        logger.trace("cacheLostMessage 3");
        super.cacheLostMessage(r, token, force);
    }

    @Override
    public synchronized void cacheLostMessage(AtmosphereResource r, boolean force) {
        logger.trace("cacheLostMessage 4");
        super.cacheLostMessage(r, force);
    }

    @Override
    public synchronized Future<Object> broadcast(Object msg) {
        logger.trace("broadcast 1");
        return super.broadcast(msg);
    }

    @Override
    public synchronized Future<Object> broadcast(Object msg, AtmosphereResource r) {
        logger.trace("broadcast 2");
        return super.broadcast(msg, r);
    }

    @Override
    public synchronized Future<Object> broadcast(Object msg, Set<AtmosphereResource> subset) {
        logger.trace("broadcast 3");
        return super.broadcast(msg, subset);
    }

    @Override
    public synchronized Future<Object> broadcastOnResume(Object msg) {
        logger.trace("broadcastOnResume 1");
        return super.broadcastOnResume(msg);
    }

    @Override
    protected synchronized void cacheAndSuspend(AtmosphereResource r) {
        logger.trace("cacheAndSuspend 1");
        super.cacheAndSuspend(r);
    }

    public String getName() {
        return name;
    }
}
