/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include <QString>
#include <QSharedPointer>
#include <functional>

namespace joynr
{
class RequestStatus;
template <class... T>
class Future;
namespace types
{
class ChannelUrlInformation;
}

/**
 * @brief The LocalChannelUrlDirectory is used within the cluster controller (message routing)
 * to map a logical channel address (channelId) to one ore more Url's. It queries the
 * ChannelUrlDirectory and stores the resulting ChannelInformation.
 *
 */
class ILocalChannelUrlDirectory
{

public:
    virtual ~ILocalChannelUrlDirectory()
    {
    }
    /**
     * @brief Register a set of Url's for a channelId.
     *
     * @param channelId
     * @param channelUrlInformation
     * @param callbackFct
     */
    virtual QSharedPointer<joynr::Future<void>> registerChannelUrls(
            const QString& channelId,
            types::ChannelUrlInformation channelUrlInformation,
            std::function<void(const joynr::RequestStatus&)> callbackFct = nullptr) = 0;

    /**
     * @brief Unregister ALL Url's registered for this channelId
     *
     * @param channelId
     * @param callbackFct
     */
    virtual QSharedPointer<joynr::Future<void>> unregisterChannelUrls(
            const QString& channelId,
            std::function<void(const joynr::RequestStatus&)> callbackFct = nullptr) = 0;

    /**
     * @brief Get ALL Url's registered in the remoteChannelUrlDirectory. Uses caching, i.e. once an
     * entry is obtained it is stored and returned from there on (instead of starting another remote
     *request).
     *
     * @param channelId
     * @param callbackFct
     */
    virtual QSharedPointer<joynr::Future<joynr::types::ChannelUrlInformation>> getUrlsForChannel(
            const QString& channelId,
            const qint64& timeout_ms,
            std::function<void(const joynr::RequestStatus&, const types::ChannelUrlInformation&)>
                    callbackFct = nullptr) = 0;
};

} // namespace joynr
#endif // ILOCALCHANNELURLDIRECTORY_H_
