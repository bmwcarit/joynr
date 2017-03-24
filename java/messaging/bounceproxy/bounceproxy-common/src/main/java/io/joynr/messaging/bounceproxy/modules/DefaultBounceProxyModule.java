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

import io.joynr.messaging.bounceproxy.LongPollingMessagingDelegate;
import io.joynr.messaging.bounceproxy.service.DefaultBounceProxyChannelServiceImpl;
import io.joynr.messaging.service.ChannelService;
import io.joynr.messaging.system.SystemTimeProvider;
import io.joynr.messaging.system.TimestampProvider;

import com.google.inject.AbstractModule;

/**
 * Default bindings for any bounceproxy implementation. This module should be
 * overridden by modules that define bindings specific to the bounceproxy.
 * 
 * @author christina.strobel
 * 
 */
public class DefaultBounceProxyModule extends AbstractModule {

    @Override
    protected void configure() {
        bind(ChannelService.class).to(DefaultBounceProxyChannelServiceImpl.class);
        bind(TimestampProvider.class).to(SystemTimeProvider.class);
        bind(LongPollingMessagingDelegate.class).asEagerSingleton();
    }

}
