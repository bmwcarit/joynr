package io.joynr.messaging.bounceproxy.controller;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::bounceproxy-controller
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

import io.joynr.messaging.bounceproxy.controller.directory.BounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.ChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.inmemory.InMemoryBounceProxyDirectory;
import io.joynr.messaging.bounceproxy.controller.directory.inmemory.InMemoryChannelDirectory;
import io.joynr.messaging.bounceproxy.controller.strategy.ChannelAssignmentStrategy;
import io.joynr.messaging.bounceproxy.controller.strategy.RoundRobinAssignmentStrategy;
import io.joynr.messaging.service.ChannelService;
import io.joynr.messaging.service.MonitoringService;

import com.google.inject.AbstractModule;

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

        bind(BounceProxyDirectory.class).to(InMemoryBounceProxyDirectory.class);
        bind(ChannelDirectory.class).to(InMemoryChannelDirectory.class);

        bind(ChannelAssignmentStrategy.class).to(RoundRobinAssignmentStrategy.class);
    }

}
