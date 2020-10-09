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

#ifndef IPROXYBUILDER_H
#define IPROXYBUILDER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "joynr/IProxyBuilderBase.h"

namespace joynr
{

class MessagingQos;
class DiscoveryQos;
class ArbitrationResult;
namespace exceptions
{
class DiscoveryException;
} // namespace exceptions

/**
 * @brief Interface to create a proxy object for the given interface T.
 */
template <class T>
class IProxyBuilder : public IProxyBuilderBase
{
public:
    ~IProxyBuilder() override = default;

    /**
     * @brief Build the proxy object
     *
     * The proxy is build and returned to the caller. The caller takes ownership of the proxy and
     * is responsible for deletion.
     * @return The proxy object
     */
    virtual std::shared_ptr<T> build() = 0;

    /**
     * @brief Build the proxy object asynchronously
     *
     * @param onSucess: Will be invoked when building the proxy succeeds. The created proxy is
     * passed as the parameter.
     * @param onError: Will be invoked when the proxy could not be created. An exception, which
     * describes the error, is passed as the parameter.
     */
    virtual void buildAsync(
            std::function<void(std::shared_ptr<T> proxy)> onSuccess,
            std::function<void(const exceptions::DiscoveryException&)> onError) noexcept = 0;

    /**
     * Build the proxy object based on arbitration result. For internal use only!
     *
     * @param arbitrationResult: discovery entries received by arbitration
     * @return The proxy object
     */
    virtual std::shared_ptr<T> build(const joynr::ArbitrationResult& arbitrationResult) = 0;

    /**
     * @brief Sets the messaging qos settings
     * @param messagingQos The message quality of service settings
     * @return The ProxyBuilder object
     */
    virtual std::shared_ptr<IProxyBuilder<T>> setMessagingQos(
            const MessagingQos& messagingQos) noexcept = 0;

    /**
     * @brief Sets the discovery qos settings
     * @param discoveryQos The discovery quality of service settings
     * @return The ProxyBuilder object
     */
    virtual std::shared_ptr<IProxyBuilder<T>> setDiscoveryQos(
            const DiscoveryQos& discoveryQos) noexcept = 0;

    /**
     * @brief Sets the GBIDs (Global Backend Identifiers) to select the backends in which the
     * provider will be discovered.<br>
     * Global discovery (if enabled in DiscoveryQos) will be done via the
     * GlobalCapabilitiesDirectory in the backend of the first provided GBID.<br>
     * By default, providers will be discovered in all backends known to the cluster controller via
     * the GlobalCapabilitiesDirectory in the default backend.
     * @param gbids A vector of GBIDs
     * @return The ProxyBuilder object
     * @throw std::invalid_argument if provided gbids vector is empty
     */
    virtual std::shared_ptr<IProxyBuilder<T>> setGbids(const std::vector<std::string>& gbids) = 0;
};

} // namespace joynr
#endif // IPROXYBUILDER_H
