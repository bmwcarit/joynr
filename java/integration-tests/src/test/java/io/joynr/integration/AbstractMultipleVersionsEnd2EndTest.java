/*
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
package io.joynr.integration;

import java.util.Properties;

import org.junit.Before;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class AbstractMultipleVersionsEnd2EndTest {

    DiscoveryQos discoveryQos;
    ProviderQos providerQos;
    JoynrRuntime runtime;

    JoynrRuntime getCcRuntime() {
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(MessagingPropertyKeys.CHANNELID, "discoverydirectory_channelid");

        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule());
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             runtimeModule).createApplication(DummyJoynrApplication.class);

        return application.getRuntime();
    }

    @Before
    public void setUp() {
        discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(1000);
        discoveryQos.setRetryIntervalMs(100);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        providerQos = new ProviderQos();
        providerQos.setScope(ProviderScope.LOCAL);

        // provider and proxy using same runtime to allow local-only communications
        runtime = getCcRuntime();
    }

}
