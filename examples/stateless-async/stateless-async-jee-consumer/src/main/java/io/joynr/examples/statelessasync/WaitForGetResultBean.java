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

import java.util.Optional;
import java.util.function.Consumer;

import jakarta.annotation.Resource;
import jakarta.ejb.Stateless;
import jakarta.enterprise.concurrent.ManagedExecutorService;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.examples.statelessasync.VehicleConfiguration;

@Stateless
public class WaitForGetResultBean {

    private static final Logger logger = LoggerFactory.getLogger(WaitForGetResultBean.class);

    @Inject
    private DataAccess dataAccess;

    @Resource(lookup = "concurrent/waitForGetResultsExecutor")
    private ManagedExecutorService managedExecutorService;

    public void waitForGetResult(String messageId,
                                 Consumer<VehicleConfiguration> callback,
                                 Consumer<Throwable> errorCallback) {
        try {
            managedExecutorService.submit(() -> {
                long startTime = System.currentTimeMillis();
                while (System.currentTimeMillis() - startTime < 2000L) {
                    Optional<VehicleConfiguration> result = dataAccess.getVehicleConfigurationForMessageId(messageId);
                    if (result.isPresent()) {
                        logger.info("Get result found: {}", result);
                        result.ifPresent(callback);
                        break;
                    } else {
                        logger.trace("No get result found for {}. Will continue waiting.", messageId);
                    }
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        logger.warn("Interrupted while waiting for GetResult for messageId {}", messageId, e);
                        break;
                    }
                }
                errorCallback.accept(new RuntimeException("Unable to get vehicle configuration for message ID "
                        + messageId + " in time."));
            });
        } catch (Exception e) {
            errorCallback.accept(e);
        }
    }
}
