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
#include "joynr/LocalChannelUrlDirectory.h"
#include "joynr/Future.h"

namespace joynr
{

joynr_logging::Logger* LocalChannelUrlDirectory::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "LocalChannelUrlDirectory");

LocalChannelUrlDirectory::LocalChannelUrlDirectory(
        MessagingSettings& messagingSettings,
        QSharedPointer<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy)
        : messagingSettings(messagingSettings),
          channelUrlDirectoryProxy(channelUrlDirectoryProxy),
          localCache()
{
    init();
    LOG_TRACE(logger, "Created ...");
}

LocalChannelUrlDirectory::~LocalChannelUrlDirectory()
{
    LOG_TRACE(logger, "Destroyed ...");
}

void LocalChannelUrlDirectory::init()
{

    // provisioning of Global Channel URL Directory URL
    types::QtChannelUrlInformation channelUrlDirectoryUrlInformation;
    QList<QString> channelUrlDirectoryUrls;
    QString channelUrlDirectoryUrl = messagingSettings.getChannelUrlDirectoryUrl();
    channelUrlDirectoryUrls << channelUrlDirectoryUrl;
    channelUrlDirectoryUrlInformation.setUrls(channelUrlDirectoryUrls);
    localCache.insert(
            messagingSettings.getChannelUrlDirectoryChannelId(), channelUrlDirectoryUrlInformation);
    LOG_TRACE(logger,
              QString("Provisioned Global Channel URL Directory URL (%1) into Local "
                      "Channel URL Directory").arg(channelUrlDirectoryUrl));

    // provisioning of Global Capabilities Directory URL
    types::QtChannelUrlInformation capabilitiesDirectoryUrlInformation;
    QList<QString> capabilitiesDirectoryUrls;
    QString capabilitiesDirectoryUrl = messagingSettings.getCapabilitiesDirectoryUrl();
    capabilitiesDirectoryUrls << capabilitiesDirectoryUrl;
    capabilitiesDirectoryUrlInformation.setUrls(capabilitiesDirectoryUrls);
    localCache.insert(messagingSettings.getCapabilitiesDirectoryChannelId(),
                      capabilitiesDirectoryUrlInformation);
    LOG_TRACE(logger,
              QString("Provisioned Global Capabilities Directory URL (%1) into Local "
                      "Channel URL Directory").arg(capabilitiesDirectoryUrl));
}

std::shared_ptr<joynr::Future<void>> LocalChannelUrlDirectory::registerChannelUrlsAsync(
        const std::string& channelId,
        types::ChannelUrlInformation channelUrlInformation,
        std::function<void(void)> onSuccess,
        std::function<void(const RequestStatus& status)> onError)
{
    LOG_INFO(logger, "registering Urls for id=" + QString::fromStdString(channelId));
    return channelUrlDirectoryProxy->registerChannelUrlsAsync(
            channelId, channelUrlInformation, onSuccess, onError);
}

std::shared_ptr<joynr::Future<void>> LocalChannelUrlDirectory::unregisterChannelUrlsAsync(
        const std::string& channelId,
        std::function<void(void)> onSuccess,
        std::function<void(const RequestStatus& status)> onError)
{
    LOG_TRACE(logger, "unregistering ALL Urls for id=" + QString::fromStdString(channelId));
    return channelUrlDirectoryProxy->unregisterChannelUrlsAsync(channelId, onSuccess, onError);
}

std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> LocalChannelUrlDirectory::
        getUrlsForChannelAsync(
                const std::string& channelId,
                const qint64& timeout_ms,
                std::function<void(const types::ChannelUrlInformation& channelUrls)> onSuccess,
                std::function<void(const RequestStatus& status)> onError)
{
    QString channelIdQT = QString::fromStdString(channelId);
    LOG_TRACE(logger, "trying to getUrlsForChannel for id=" + channelIdQT);

    if (localCache.contains(channelIdQT)) {
        LOG_TRACE(logger, "using cached Urls for id=" + channelIdQT);
        RequestStatus status;
        status.setCode(RequestStatusCode::OK);
        std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> future(
                new joynr::Future<joynr::types::ChannelUrlInformation>());
        future->onSuccess(types::QtChannelUrlInformation::createStd(localCache.value(channelIdQT)));
        if (onSuccess) {
            onSuccess(types::QtChannelUrlInformation::createStd(localCache.value(channelIdQT)));
        }
        return future;
    }
    assert(!channelUrlDirectoryProxy.isNull());
    std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> future(
            channelUrlDirectoryProxy->getUrlsForChannelAsync(channelId, onSuccess, onError));
    future->waitForFinished(timeout_ms);

    if (future->getStatus().successful()) {
        LOG_INFO(logger, "Received remote url information for channelId=" + channelIdQT);
        joynr::types::ChannelUrlInformation urls;
        future->getValues(urls);
        localCache.insert(channelIdQT, types::QtChannelUrlInformation::createQt(urls));
        LOG_INFO(logger, "Stored url information for channelId=" + channelIdQT);
    } else {
        LOG_INFO(logger,
                 "FAILED to receive remote url information for channelId=" + channelIdQT +
                         " . Status: " + future->getStatus().toString());
    }
    return future;
}

} // namespace joynr
