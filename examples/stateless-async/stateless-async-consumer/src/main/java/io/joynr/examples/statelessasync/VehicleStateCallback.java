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

import java.util.HashMap;
import java.util.Map;
import java.util.Optional;
import java.util.function.BiConsumer;

import io.joynr.proxy.ReplyContext;
import joynr.examples.statelessasync.VehicleConfiguration;
import joynr.examples.statelessasync.VehicleState;
import joynr.examples.statelessasync.VehicleStateStatelessAsyncCallback;

public class VehicleStateCallback implements VehicleStateStatelessAsyncCallback {

    private Map<String, Runnable> addConfigurationCallbacks;
    private Map<String, BiConsumer<VehicleConfiguration, VehicleState.GetCurrentConfigErrorEnum>> getCurrentConfigCallbacks;

    public VehicleStateCallback(Map<String, Runnable> addConfigurationCallbacks,
                                Map<String, BiConsumer<VehicleConfiguration, VehicleState.GetCurrentConfigErrorEnum>> getCurrentConfigCallbacks) {
        this.addConfigurationCallbacks = (addConfigurationCallbacks != null) ? new HashMap<>(addConfigurationCallbacks)
                : null;
        this.getCurrentConfigCallbacks = (getCurrentConfigCallbacks != null) ? new HashMap<>(getCurrentConfigCallbacks)
                : null;
    }

    @Override
    public String getUseCase() {
        return "pure-java-proxy";
    }

    @Override
    public void getCurrentConfigSuccess(VehicleConfiguration result, ReplyContext replyContext) {
        Optional.ofNullable(getCurrentConfigCallbacks.get(replyContext.getMessageId()))
                .orElseThrow(() -> new RuntimeException("Unknown messageId " + replyContext
                        + " for getCurrentConfiguration success callback"))
                .accept(result, null);
    }

    @Override
    public void getCurrentConfigFailed(VehicleState.GetCurrentConfigErrorEnum error, ReplyContext replyContext) {
        Optional.ofNullable(getCurrentConfigCallbacks.get(replyContext.getMessageId()))
                .orElseThrow(() -> new RuntimeException("Unknown messageId " + replyContext
                        + " for getCurrentConfiguration failed callback"))
                .accept(null, error);
    }

    @Override
    public void addConfigurationSuccess(ReplyContext replyContext) {
        Optional.ofNullable(addConfigurationCallbacks.get(replyContext.getMessageId()))
                .orElseThrow(() -> new RuntimeException("Unknown messageId " + replyContext
                        + " for addConfiguration success callback"))
                .run();
    }
}
