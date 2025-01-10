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
package io.joynr.examples.statelessasync.jee.carsim;

import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

import jakarta.ejb.Singleton;
import jakarta.inject.Inject;

import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.examples.statelessasync.VehicleStateSubscriptionPublisher;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.examples.statelessasync.VehicleConfiguration;
import joynr.examples.statelessasync.VehicleStateSync;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;

@Singleton
@ServiceProvider(serviceInterface = VehicleStateSync.class)
public class VehicleStateServiceBean implements VehicleStateSync {

    private static final Logger logger = LoggerFactory.getLogger(VehicleStateServiceBean.class);

    private Map<String, VehicleConfiguration> configurations = new HashMap<>();

    @Inject
    @SubscriptionPublisher
    private VehicleStateSubscriptionPublisher vehicleStateSubscriptionPublisher;

    @Override
    public Integer getNumberOfConfigs() {
        logger.info("getNumberOfConfigs called");
        return configurations.size();
    }

    @Override
    public VehicleConfiguration getCurrentConfig(String id) throws ApplicationException {
        logger.info("getCurrentConfig called with {}", id);
        return configurations.get(id);
    }

    @Override
    public void addConfiguration(VehicleConfiguration configuration) {
        logger.info("addConfiguration called with {}", configuration);
        configurations.put(configuration.getId(), configuration);
        vehicleStateSubscriptionPublisher.numberOfConfigsChanged(configurations.size());
    }

    @Override
    public void callWithExceptionTest(String addToExceptionMessage) {
        logger.info("callWithExceptionTest called");
        throw new ProviderRuntimeException("Test " + UUID.randomUUID().toString());
    }

    @Override
    public void callFireAndForgetTest(String dataIn) {
        logger.info("Received {}", dataIn);
    }

}
