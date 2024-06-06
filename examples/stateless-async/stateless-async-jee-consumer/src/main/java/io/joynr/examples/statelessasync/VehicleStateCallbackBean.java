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

import jakarta.ejb.Stateless;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.api.CallbackHandler;
import io.joynr.proxy.ReplyContext;
import joynr.examples.statelessasync.VehicleConfiguration;
import joynr.examples.statelessasync.VehicleState;
import joynr.examples.statelessasync.VehicleStateStatelessAsyncCallback;

@Stateless
@CallbackHandler
public class VehicleStateCallbackBean implements VehicleStateStatelessAsyncCallback {

    private static final Logger logger = LoggerFactory.getLogger(VehicleStateCallbackBean.class);

    @Inject
    private DataAccess dataAccess;

    @Override
    public String getUseCase() {
        return "jee-consumer-test";
    }

    @Override
    public void getCurrentConfigSuccess(VehicleConfiguration result, ReplyContext replyContext) {
        logger.info("Got config for {}:\n{}", replyContext, result);
        dataAccess.updateGetResult(replyContext.getMessageId(), result);
    }

    @Override
    public void getCurrentConfigFailed(VehicleState.GetCurrentConfigErrorEnum error, ReplyContext replyContext) {
        logger.error("Unable to get config {} / {}", replyContext, error);
    }

    @Override
    public void addConfigurationSuccess(ReplyContext replyContext) {
        logger.info("Successfully persisted {}", replyContext);
        dataAccess.updateKnownConfiguration(replyContext.getMessageId(), true);
    }

    @Override
    public void getNumberOfConfigsSuccess(Integer numberOfConfigs, ReplyContext replyContext) {
        logger.info("Number of configs for message ID {} is {}", replyContext, numberOfConfigs);
    }

    @Override
    public void callWithExceptionTestFailed(JoynrRuntimeException runtimeException, ReplyContext replyContext) {
        logger.info("Expected failure occurred: {} for {}", runtimeException, replyContext);
    }
}
