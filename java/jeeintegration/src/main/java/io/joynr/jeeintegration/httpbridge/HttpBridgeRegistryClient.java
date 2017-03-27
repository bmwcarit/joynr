/**
 *
 */
package io.joynr.jeeintegration.httpbridge;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import java.util.concurrent.CompletionStage;

/**
 * Interface used for communicating with the HTTP bridge (which forwards messages received via MQTT at a broker to this
 * application via HTTP).
 */
public interface HttpBridgeRegistryClient {

    /**
     * Registers the given endpoint URL as the recipient for messages received in the broker at the given topic.
     * <p>
     * Implementations should prefer to perform the registration asynchronously and provide a completion stage as
     * result, which can be used by the caller to specify any actions which need to be taken upon completion
     * (successfully or otherwise) of the registration.
     * </p>
     *
     * @param endpointUrl
     *            the endpoint URL to forward messages to.
     * @param channelId
     *            the channel ID which will be used to calculate the topic for
     *
     * @return a completion stage which can be used to define further actions to take upon successful or exceptional
     *         completion of the registration.
     */
    CompletionStage<Void> register(String endpointUrl, String channelId);

}
