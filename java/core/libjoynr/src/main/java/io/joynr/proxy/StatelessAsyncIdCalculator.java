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

import java.lang.reflect.Method;

public interface StatelessAsyncIdCalculator {
    /**
     * Use this method to calculate the participant ID to use for requests for stateless async methods, so that the
     * replies can be routed back to any participant in a cluster making stateless async calls, rather than just the
     * one node it originates from.
     *
     * Implementations must ensure that the result can be used as input to {@link #fromParticipantUuid(String)}
     * and then interpreted by any node in a cluster. Either by requiring
     * the same config to be used by each node, or by sharing information at runtime between the nodes.
     *
     * @param interfaceName the joynr service interface for which the ID is to be generated.
     * @param statelessAsyncCallback the stateless callback instance which is used for processing the result. Each node
     *                               must register an instance created in the same way which behave identically at startup.
     * @return a type 3 UUID participant ID to be used for stateless async requests.
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

    /**
     * This method returns the unique ID of the callback method to use for handling the reply. See also
     * {@link io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation}.
     *
     * The method ID is ascertained by looking for the correlation annotation on the method, which during generation is
     * calculated from the callback method signature and added to both the service interface and the callback interface
     * methods.
     *
     * @param method the method being called for which the ID of the callback method should be added to the result.
     * @return the unique method ID of the stateless async callback method to use for handling replies.
     */
    String calculateStatelessCallbackMethodId(Method method);

    /**
     * Calculates a random request/reply ID with the method ID from the callback appended to it.
     * The two will be concatenated using the '#' character.
     *
     * In order to be able to transmit the method ID without changing the joynr message format or request / reply
     * payload objects, we use the request/reply ID in order to convey the required information.
     *
     * @param method the method from which to read the correlation ID (see also {@link #calculateStatelessCallbackMethodId(Method)})
     * @return the request/reply ID with the callback method ID appended
     */
    String calculateStatelessCallbackRequestReplyId(Method method);

    /**
     * Extracts the callback method ID from a request/reply ID previously generated with
     * {@link #calculateStatelessCallbackRequestReplyId(Method)}.
     *
     * @param requestReplyId the request/reply ID from which to extract the callback method ID
     * @return the method ID
     */
    String extractMethodIdFromRequestReplyId(String requestReplyId);

    /**
     * Obtains the stateless callback ID from the participant UUID previously created with
     * {@link #calculateParticipantId(String, StatelessAsyncCallback)}
     * which can then be added to the {@link joynr.Reply#setStatelessCallback(String) reply}.
     *
     * @param statelessParticipantId the particpant ID
     * @return the stateless callback ID which can be used to lookup the relevant callback.
     */
    String fromParticipantUuid(String statelessParticipantId);
}
