/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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
#ifndef ICLUSTERCONTROLLERSIGNALHANDLER_H
#define ICLUSTERCONTROLLERSIGNALHANDLER_H

namespace joynr
{
class IClusterControllerSignalHandler
{
public:
    virtual ~IClusterControllerSignalHandler() = default;

    /**
     * @brief Start all external messaging communication channels.
     * External communication is meant everything that leaves the cluster-controller
     * towards a MQTT broker or HTTP bounce-proxy.
     * Local communications running over WebSocket, are not affected.
     * Triggered on POSIX systems with SIGUSR1.
     */
    virtual void startExternalCommunication() = 0;

    /**
     * @brief Stop all external messaging communication channels.
     * External communication is meant everything that leaves the cluster-controller
     * towards a MQTT broker or HTTP bounce-proxy.
     * Local communications running over WebSocket, are not affected.
     * Triggered on POSIX systems with SIGUSR2.
     */
    virtual void stopExternalCommunication() = 0;

    /**
     * @brief Shutdown the cluster controller.
     * Triggered on POSIX systems with SIGTERM.
     */
    virtual void shutdownClusterController() = 0;
};
} // namespace joynr

#endif // ICLUSTERCONTROLLERSIGNALHANDLER_H
