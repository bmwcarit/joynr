/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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
package io.joynr.proxy;

public interface StatelessAsyncIdCalculator {
    /**
     * Use this method to calculate the participant ID to use for requests for stateless async methods, so that the
     * replies can be routed back to any participant in a cluster making stateless async calls, rather than just the
     * one node it originates from.
     *
     * Implementations must ensure that the result can be interpreted by any node in a cluster. Either by requiring
     * the same config to be used by each node, or by sharing information at runtime between the nodes.
     *
     * @param interfaceName the joynr service interface for which the ID is to be generated.
     * @param statelessAsyncCallback the stateless callback instance which is used for processing the result. Each node
     *                               must register an instance created in the same way which behave identically at startup.
     * @return the participant ID to be used for stateless async requests.
     */
    String calculateParticipantId(String interfaceName, StatelessAsyncCallback statelessAsyncCallback);

    /**
     * This method will calculate an identifier for a given stateless callback instance using the interface name of the
     * joynr service and the use case of the stateless async callback instance in order to allow the joynr system to
     * identify which callback instance should be used for a reply to a stateless async request.
     *
     * @param interfaceName the joynr service interface for which the ID is to be generated.
     * @param statelessAsyncCallback the stateless callback instance which is used for processing the result. Each node
     *                               must register an instance created in the same way which behave identically at startup.
     * @return a string which uniquely identifies a given stateless async callback.
     */
    String calculateStatelessCallbackId(String interfaceName, StatelessAsyncCallback statelessAsyncCallback);
}
