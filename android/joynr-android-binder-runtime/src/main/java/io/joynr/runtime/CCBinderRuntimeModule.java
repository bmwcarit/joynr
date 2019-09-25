/*-
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import android.content.Context;

import com.google.inject.Provides;
import com.google.inject.Singleton;

import javax.inject.Named;

import io.joynr.android.AndroidBinderRuntime;
import io.joynr.android.messaging.binder.BinderClientMessagingStubFactory;
import io.joynr.android.messaging.binder.BinderMessagingSkeletonFactory;
import io.joynr.messaging.inprocess.InProcessAddress;
import joynr.system.RoutingTypes.Address;
import joynr.system.RoutingTypes.BinderAddress;

/**
 * Binder runtime guice module that implements the necessary dependencies for having a
 * joynr cluster controller using android binder communication mechanism.
 */
public class CCBinderRuntimeModule extends ClusterControllerRuntimeModule {

    Context context;

    public CCBinderRuntimeModule(Context context) {
        this.context = context;
    }

    @Override
    protected void configure() {
        super.configure();
        bind(JoynrRuntime.class).to(ClusterControllerRuntime.class);
        bind(ClusterControllerRuntime.class).in(Singleton.class);

        messagingStubFactory.addBinding(BinderAddress.class).to(BinderClientMessagingStubFactory.class);
        messagingSkeletonFactory.addBinding(BinderAddress.class).to(BinderMessagingSkeletonFactory.class);
    }

    @Provides
    @Singleton
    @Named(SystemServicesSettings.PROPERTY_CC_MESSAGING_ADDRESS)
    Address provideCCMessagingAddress() {
        return new InProcessAddress();
    }

    @Provides
    @Singleton
    @Named(AndroidBinderRuntime.PROPERTY_CONTEXT_ANDROID)
    Context provideContext() {
        return context;
    }

}
