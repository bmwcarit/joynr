package io.joynr.messaging.bounceproxy.modules;

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

import io.joynr.messaging.bounceproxy.BounceProxyBroadcaster;

import java.util.HashMap;
import java.util.Map;

import org.atmosphere.cache.UUIDBroadcasterCache;

import com.google.inject.AbstractModule;
import com.google.inject.TypeLiteral;
import com.google.inject.name.Names;

/**
 * Module to inject Atmosphere framework related configuration parameters.
 * 
 * @author christina.strobel
 * 
 */
public class AtmosphereModule extends AbstractModule {

    private final Map<String, String> params;

    public AtmosphereModule() {
        this.params = new HashMap<String, String>();
    }

    @Override
    protected void configure() {

        params.put("suspend.seconds", "20");
        params.put("org.atmosphere.cpr.broadcasterClass", BounceProxyBroadcaster.class.getName());
        params.put("org.atmosphere.cpr.broadcasterCacheClass", UUIDBroadcasterCache.class.getName());
        params.put("org.atmosphere.useBlocking", "false");
        params.put("org.atmosphere.cpr.broadcasterLifeCyclePolicy", "NEVER");
        params.put("org.atmosphere.cpr.broadcaster.shareableThreadPool", "true");
        params.put("com.sun.jersey.config.feature.DisableWADL", "true");
        params.put("org.atmosphere.cpr.BroadcasterCache.strategy", "beforeFilter");

        bind(new TypeLiteral<Map<String, String>>() {
        }).annotatedWith(Names.named("org.atmosphere.guice.AtmosphereGuiceServlet.properties")).toInstance(params);
    }

    /**
     * Gets the parameters that the Atmosphere framework is configured with.
     * These are parameters that would be used as init-params in web.xml if not
     * configured with Guice.
     * 
     * @return Parameter Map with key-value pairs
     */
    public Map<String, String> getParameters() {
        return params;
    }

}
