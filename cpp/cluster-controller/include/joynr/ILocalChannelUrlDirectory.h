/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
#ifndef ILOCALCHANNELURLDIRECTORY_H_
#define ILOCALCHANNELURLDIRECTORY_H_
#include <memory>
#include <functional>
#include <string>
#include "joynr/exceptions/JoynrException.h"
#include <chrono>

namespace joynr
{
class RequestStatus;
template <class... T>
class Future;
namespace types
{
class ChannelUrlInformation;
} // namespace types

/**
 * @brief The LocalChannelUrlDirectory is used within the cluster controller (message routing)
 * to map a logical channel address (channelId) to one ore more Url's. It queries the
 * ChannelUrlDirectory and stores the resulting ChannelInformation.
 *
 */
class ILocalChannelUrlDirectory
{

public:
    virtual ~ILocalChannelUrlDirectory() = default;
    /**
     * @brief Register a set of Url's for a channelId.
     *
     * @param channelId
     * @param channelUrlInformation
     * @param onSuccess
     * @param onError
     */
    virtual std::shared_ptr<Future<void>> registerChannelUrlsAsync(
            const std::string& channelId,
            types::ChannelUrlInformation channelUrlInformation,
            std::function<void(void)> onSuccess = nullptr,
            std::function<void(const exceptions::JoynrException&)> onError = nullptr) = 0;

    /**
     * @brief Unregister ALL Url's registered for this channelId
     *
     * @param channelId
     * @param onSuccess
     * @param onError
     */
    virtual std::shared_ptr<Future<void>> unregisterChannelUrlsAsync(
            const std::string& channelId,
            std::function<void(void)> onSuccess = nullptr,
            std::function<void(const exceptions::JoynrException&)> onError = nullptr) = 0;

    /**
     * @brief Get ALL Url's registered in the remoteChannelUrlDirectory. Uses caching, i.e. once an
     * entry is obtained it is stored and returned from there on (instead of starting another remote
     *request).
     *
     * @param channelId
     * @param onSuccess
     * @param onError
     */
    virtual std::shared_ptr<Future<joynr::types::ChannelUrlInformation>> getUrlsForChannelAsync(
            const std::string& channelId,
            std::chrono::milliseconds timeout,
            std::function<void(const types::ChannelUrlInformation&)> onSuccess = nullptr,
            std::function<void(const exceptions::JoynrException&)> onError = nullptr) = 0;
};

} // namespace joynr
#endif // ILOCALCHANNELURLDIRECTORY_H_
