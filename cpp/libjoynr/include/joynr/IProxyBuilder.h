/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

namespace joynr
{

class MessagingQos;
class DiscoveryQos;

/**
 * @brief Interface to create a proxy object for the given interface T.
 */
template <class T>
class IProxyBuilder
{
public:
    virtual ~IProxyBuilder() = default;

    /**
     * @brief Build the proxy object
     *
     * The proxy is build and returned to the caller. The caller takes ownership of the proxy and
     * is responsible for deletion.
     * @return The proxy object
     */
    virtual T* build() = 0;

    /**
     * @brief Sets whether the object is to be cached
     * @param cached True, if the object is to be cached, false otherwise
     * @return The ProxyBuilder object
     */
    virtual IProxyBuilder* setCached(const bool cached) = 0;

    /**
     * @brief Sets the messaging qos settings
     * @param messagingQos The message quality of service settings
     * @return The ProxyBuilder object
     */
    virtual IProxyBuilder* setMessagingQos(const MessagingQos& messagingQos) = 0;

    /**
     * @brief Sets the discovery qos settings
     * @param discoveryQos The discovery quality of service settings
     * @return The ProxyBuilder object
     */
    virtual IProxyBuilder* setDiscoveryQos(const DiscoveryQos& discoveryQos) = 0;
};

} // namespace joynr
#endif // IPROXYBUILDER_H
