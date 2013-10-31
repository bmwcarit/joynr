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

import java.util.concurrent.TimeUnit;

import org.atmosphere.cache.UUIDBroadcasterCache;
import org.atmosphere.cpr.AtmosphereConfig;
import org.atmosphere.cpr.DefaultBroadcaster;

public class BounceProxyBroadcaster extends DefaultBroadcaster {

    public BounceProxyBroadcaster(String name, AtmosphereConfig config) {
        super(name, config);

        UUIDBroadcasterCache broadcasterCache = (UUIDBroadcasterCache) bc.getBroadcasterCache();
        broadcasterCache.setClientIdleTime(TimeUnit.DAYS.toMillis(7));

        // order of filters matters
        bc.addFilter(new ExpirationFilter());
        bc.addFilter(new MessageSerializationFilter());

    }

    public String getName() {
        return name;
    }
}
