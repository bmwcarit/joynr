package io.joynr.messaging.bounceproxy;

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

import io.joynr.messaging.bounceproxy.filter.ExpirationFilter;
import io.joynr.messaging.bounceproxy.filter.MessageSerializationFilter;

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
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public void cacheLostMessage(AtmosphereResource r) {
        synchronized (resources) {
            logger.trace("cacheLostMessage 1");
            super.cacheLostMessage(r);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public void cacheLostMessage(AtmosphereResource r, AsyncWriteToken token) {
        synchronized (resources) {
            logger.trace("cacheLostMessage 2");
            super.cacheLostMessage(r, token);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public void cacheLostMessage(AtmosphereResource r, AsyncWriteToken token, boolean force) {
        synchronized (resources) {
            logger.trace("cacheLostMessage 3");
            super.cacheLostMessage(r, token, force);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public void cacheLostMessage(AtmosphereResource r, boolean force) {
        synchronized (resources) {
            logger.trace("cacheLostMessage 4");
            super.cacheLostMessage(r, force);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public Future<Object> broadcast(Object msg) {
        synchronized (resources) {
            logger.trace("broadcast 1");
            return super.broadcast(msg);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public Future<Object> broadcast(Object msg, AtmosphereResource r) {
        synchronized (resources) {
            logger.trace("broadcast 2");
            return super.broadcast(msg, r);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public Future<Object> broadcast(Object msg, Set<AtmosphereResource> subset) {
        synchronized (resources) {
            logger.trace("broadcast 3");
            return super.broadcast(msg, subset);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    public Future<Object> broadcastOnResume(Object msg) {
        synchronized (resources) {
            logger.trace("broadcastOnResume 1");
            return super.broadcastOnResume(msg);
        }
    }

    @Override
    @edu.umd.cs.findbugs.annotations.SuppressWarnings(
                                                      value = "JLM_JSR166_UTILCONCURRENT_MONITORENTER",
                                                      justification = "Can only sync on resources because that is what Atomosphere is doing in super")
    protected void cacheAndSuspend(AtmosphereResource r) {
        synchronized (resources) {
            logger.trace("cacheAndSuspend 1");
            super.cacheAndSuspend(r);
        }
    }

    public String getName() {
        return name;
    }
}
