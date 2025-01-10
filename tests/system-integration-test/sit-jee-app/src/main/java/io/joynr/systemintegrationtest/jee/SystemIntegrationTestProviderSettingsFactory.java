/*
 * #%L
 * %%
 * Copyright (C) 2025 BMW Car IT GmbH
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
package io.joynr.systemintegrationtest.jee;

import jakarta.ejb.Singleton;

import io.joynr.jeeintegration.api.ProviderRegistrationSettingsFactory;
import joynr.test.SystemIntegrationTestSync;
import joynr.types.ProviderQos;

@Singleton
public class SystemIntegrationTestProviderSettingsFactory implements ProviderRegistrationSettingsFactory {

    @Override
    public ProviderQos createProviderQos() {
        ProviderQos providerQos = new ProviderQos();
        return providerQos;
    }

    @Override
    public String[] createGbids() {
        // Make sure these GBIDs are valid and are part of ConfigurableMessagingSettings.PROPERTY_GBIDS
        String[] gbidsForRegistration = System.getenv("SIT_GBIDS").trim().split(",");
        return gbidsForRegistration;
    }

    @Override
    public boolean providesFor(Class<?> serviceInterface) {
        return SystemIntegrationTestSync.class.equals(serviceInterface);
    }

}
