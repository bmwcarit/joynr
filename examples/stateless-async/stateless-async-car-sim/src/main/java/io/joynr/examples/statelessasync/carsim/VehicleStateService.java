/*-
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.examples.statelessasync.carsim;

import java.util.HashMap;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.examples.statelessasync.DefaultVehicleStateProvider;
import joynr.examples.statelessasync.VehicleConfiguration;
import joynr.examples.statelessasync.VehicleState;

public class VehicleStateService extends DefaultVehicleStateProvider {

    private static final Logger logger = LoggerFactory.getLogger(VehicleStateService.class);

    private Map<String, VehicleConfiguration> vehicleConfigurationMap = new HashMap<>();

    @Override
    public Promise<DeferredVoid> addConfiguration(VehicleConfiguration configuration) {
        vehicleConfigurationMap.put(configuration.getId(), configuration);
        numberOfConfigs = vehicleConfigurationMap.size();
        DeferredVoid deferredVoid = new DeferredVoid();
        deferredVoid.resolve();
        return new Promise<>(deferredVoid);
    }

    @Override
    public Promise<GetCurrentConfigDeferred> getCurrentConfig(String id) {
        GetCurrentConfigDeferred result = new GetCurrentConfigDeferred();
        if (!vehicleConfigurationMap.containsKey(id)) {
            result.reject(VehicleState.GetCurrentConfigErrorEnum.UNKNOWN_CONFIGURATION_ID);
        } else {
            result.resolve(vehicleConfigurationMap.get(id));
        }
        return new Promise<>(result);
    }

    @Override
    public Promise<DeferredVoid> callWithExceptionTest(String addToExceptionMessage) {
        throw new ProviderRuntimeException("Failed with " + addToExceptionMessage);
    }

    @Override
    public void callFireAndForgetTest(String dataIn) {
        logger.info("Fire and forget method called with: {}", dataIn);
    }

    public void printCurrentConfigs() {
        logger.info("Configs are currently:\n{}\n\n", vehicleConfigurationMap);
    }
}
