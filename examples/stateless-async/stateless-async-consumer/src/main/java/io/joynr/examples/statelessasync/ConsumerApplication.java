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
package io.joynr.examples.statelessasync;

import static io.joynr.util.JoynrUtil.createUuidString;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.function.BiConsumer;
import java.util.stream.IntStream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.examples.statelessasync.KeyValue;
import joynr.examples.statelessasync.VehicleConfiguration;
import joynr.examples.statelessasync.VehicleState;
import joynr.examples.statelessasync.VehicleStateProxy;
import joynr.examples.statelessasync.VehicleStateStatelessAsync;

public class ConsumerApplication extends AbstractJoynrApplication {

    private static final Logger logger = LoggerFactory.getLogger(ConsumerApplication.class);

    @Override
    public void run() {
        logger.info("ConsumerApplication running ...");

        Map<String, BiConsumer<VehicleConfiguration, VehicleState.GetCurrentConfigErrorEnum>> getConfigurationCallbacks = new HashMap<>();
        Map<String, Runnable> addConfigurationCallbacks = new HashMap<>();
        VehicleStateCallback vehicleStateCallback = new VehicleStateCallback(addConfigurationCallbacks,
                                                                             getConfigurationCallbacks);
        runtime.registerStatelessAsyncCallback(vehicleStateCallback);
        ProxyBuilder<VehicleStateProxy> proxyBuilder = runtime.getProxyBuilder("io.joynr.examples.statelessasync.carsim",
                                                                               VehicleStateProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setDiscoveryTimeoutMs(60000L);
        discoveryQos.setRetryIntervalMs(500L);
        proxyBuilder.setDiscoveryQos(discoveryQos);
        proxyBuilder.setStatelessAsyncCallbackUseCase(vehicleStateCallback.getUseCase());
        VehicleStateStatelessAsync vehicleStateProxy = proxyBuilder.build();

        int iterations = 10;
        CountDownLatch countDownLatch = new CountDownLatch(iterations);
        IntStream.range(0, iterations).forEach(counter -> {
            VehicleConfiguration vehicleConfiguration = new VehicleConfiguration();
            vehicleConfiguration.setId(createUuidString());
            vehicleConfiguration.setEntries(new KeyValue[]{ new KeyValue("key-" + counter, "value-" + counter) });
            vehicleStateProxy.addConfiguration(vehicleConfiguration, addConfigurationMessageId -> {
                logger.info("Message sent with ID: {}", addConfigurationMessageId);
                vehicleStateCallback.setAddConfigurationCallback(addConfigurationMessageId, () -> {
                    logger.info("Add configuration reply for message {}", addConfigurationMessageId);
                    vehicleStateProxy.getCurrentConfig(vehicleConfiguration.getId(), getConfigurationMessageId -> {
                        vehicleStateCallback.setGetCurrentConfigCallback(getConfigurationMessageId, (config, error) -> {
                            logger.info("Get configuration reply for message {}", getConfigurationMessageId);
                            if (config != null && !config.getId().equals(vehicleConfiguration.getId())) {
                                logger.error("Mismatched config IDs: " + vehicleConfiguration + " / " + config);
                            } else if (config != null) {
                                logger.info("Retrieved config: {}", config);
                            }
                            if (error != null) {
                                logger.info("getConfig for ID " + vehicleConfiguration.getId() + " resulted in error: "
                                        + error);
                            }
                            countDownLatch.countDown();
                        });
                    });
                });
            });
        });
        try {
            countDownLatch.await(2, TimeUnit.MINUTES);
        } catch (Exception e) {
            logger.error("Unable to successfully make calls.", e);
        }
    }

}
