/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.runtime;

import java.util.HashMap;
import java.util.Map;

import com.google.inject.AbstractModule;
import com.google.inject.Provides;
import com.google.inject.Singleton;
import com.google.inject.name.Named;

import io.joynr.messaging.AbstractMiddlewareMessagingStubFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import io.joynr.test.TestRuntime;
import joynr.system.RoutingTypes.Address;

public class TestRuntimeModule extends AbstractModule {
    @Override
    protected void configure() {
        bind(JoynrRuntime.class).to(TestRuntime.class).in(Singleton.class);
    }

    @Provides
    @Singleton
    Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> provideMessagingStubFactories() {
        Map<Class<? extends Address>, AbstractMiddlewareMessagingStubFactory> factories = new HashMap<>();
        return factories;
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    public Address provideCCMessagingAddress() {
        return new InProcessAddress();
    }
}
