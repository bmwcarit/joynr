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
#ifndef LOCALCHANNELURLDIRECTORY_H_
#define LOCALCHANNELURLDIRECTORY_H_
#include "joynr/PrivateCopyAssign.h"
#include "joynr/ILocalChannelUrlDirectory.h"
#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/joynrlogging.h"
#include "joynr/infrastructure/ChannelUrlDirectoryProxy.h"
#include "joynr/types/StdChannelUrlInformation.h"
#include "joynr/MessagingSettings.h"
#include <string>

namespace joynr
{

/**
 * @brief The LocalChannelUrlDirectory is used within the cluster controller (message routing)
 * to map a logical channel address (channelId) to one ore more Url's. It queries the
 * ChannelUrlDirectory and stores the resulting ChannelInformation.
 *
 */
class JOYNRCLUSTERCONTROLLER_EXPORT LocalChannelUrlDirectory : public ILocalChannelUrlDirectory
{

public:
    LocalChannelUrlDirectory(
            MessagingSettings& messagingSettings,
            QSharedPointer<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy);

    virtual ~LocalChannelUrlDirectory();

    /**
     * @brief Register a set of Url's for a channelId.
     *
     * @param channelId
     * @param channelUrlInformation
     * @param callbackFct
     */
    virtual QSharedPointer<joynr::Future<void>> registerChannelUrls(
            const std::string& channelId,
            types::StdChannelUrlInformation channelUrlInformation,
            std::function<void(const RequestStatus& status)> callbackFct = nullptr);

    /**
     * @brief Unregister ALL Url's registered for this channelId
     *
     * @param channelId
     * @param callbackFct
     */
    virtual QSharedPointer<joynr::Future<void>> unregisterChannelUrls(
            const std::string& channelId,
            std::function<void(const RequestStatus& status)> callbackFct = nullptr);

    /**
     * @brief Get ALL Url's registered in the remoteChannelUrlDirectory. Uses caching, i.e. once an
     * entry is obtained it is stored and returned from there on (instead of starting another remote
     *request).
     *
     * @param channelId
     * @param timeout
     * @param callbackFct
     */
    virtual QSharedPointer<joynr::Future<joynr::types::StdChannelUrlInformation>> getUrlsForChannel(
            const std::string& channelId,
            const qint64& timeout_ms,
            std::function<void(const RequestStatus& status,
                               const types::StdChannelUrlInformation& channelUrls)> = nullptr);

private:
    DISALLOW_COPY_AND_ASSIGN(LocalChannelUrlDirectory);
    void init();
    MessagingSettings& messagingSettings;
    QSharedPointer<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy;
    QMap<QString, types::ChannelUrlInformation> localCache;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif // LOCALCHANNELURLDIRECTORY_H_
