package io.joynr.messaging.bounceproxy.controller;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.bounceproxy.controller.strategy.ChannelAssignmentStrategy;
import io.joynr.messaging.bounceproxy.controller.strategy.RoundRobinAssignmentStrategy;
import io.joynr.messaging.service.ChannelService;
import io.joynr.messaging.service.MonitoringService;
import io.joynr.messaging.system.SystemTimeProvider;
import io.joynr.messaging.system.TimestampProvider;

import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;

/**
 * GUICE module to configure dependency injection for the bounce proxy
 * controller.
 * 
 * @author christina.strobel
 * 
 */
public class BounceProxyControllerModule extends AbstractModule {

    /*
     * (non-Javadoc)
     * 
     * @see com.google.inject.AbstractModule#configure()
     */
    @Override
    protected void configure() {

        bind(ChannelService.class).to(ChannelServiceImpl.class);
        bind(MonitoringService.class).to(MonitoringServiceImpl.class);

        bind(TimestampProvider.class).to(SystemTimeProvider.class);

        bind(ChannelAssignmentStrategy.class).to(RoundRobinAssignmentStrategy.class);
    }

    @Provides
    CloseableHttpClient getCloseableHttpClient() {
        // For now, we provide the httpclient in here, as the configuration is
        // different from messaging configuration (e.g. no proxy settings).
        // If a custom configuration is needed, a separate provider should be
        // created.
        return HttpClients.createDefault();
    }
}
