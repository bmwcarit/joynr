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
#include "joynr/LocalChannelUrlDirectory.h"
#include "joynr/Future.h"
#include "joynr/QtTypeUtil.h"

namespace joynr
{

joynr_logging::Logger* LocalChannelUrlDirectory::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "LocalChannelUrlDirectory");

LocalChannelUrlDirectory::LocalChannelUrlDirectory(
        MessagingSettings& messagingSettings,
        std::shared_ptr<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy)
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
    types::ChannelUrlInformation channelUrlDirectoryUrlInformation;
    std::vector<std::string> channelUrlDirectoryUrls;
    std::string channelUrlDirectoryUrl = messagingSettings.getChannelUrlDirectoryUrl();
    channelUrlDirectoryUrls.push_back(channelUrlDirectoryUrl);
    channelUrlDirectoryUrlInformation.setUrls(channelUrlDirectoryUrls);
    localCache.insert(QtTypeUtil::toQt(messagingSettings.getChannelUrlDirectoryChannelId()),
                      channelUrlDirectoryUrlInformation);
    LOG_TRACE(logger,
              FormatString("Provisioned Global Channel URL Directory URL (%1) into Local "
                           "Channel URL Directory")
                      .arg(channelUrlDirectoryUrl)
                      .str());

    // provisioning of Global Capabilities Directory URL
    types::ChannelUrlInformation capabilitiesDirectoryUrlInformation;
    std::vector<std::string> capabilitiesDirectoryUrls;
    std::string capabilitiesDirectoryUrl = messagingSettings.getCapabilitiesDirectoryUrl();
    capabilitiesDirectoryUrls.push_back(capabilitiesDirectoryUrl);
    capabilitiesDirectoryUrlInformation.setUrls(capabilitiesDirectoryUrls);
    localCache.insert(QtTypeUtil::toQt(messagingSettings.getCapabilitiesDirectoryChannelId()),
                      capabilitiesDirectoryUrlInformation);
    LOG_TRACE(logger,
              FormatString("Provisioned Global Capabilities Directory URL (%1) into Local "
                           "Channel URL Directory")
                      .arg(capabilitiesDirectoryUrl)
                      .str());
}

std::shared_ptr<joynr::Future<void>> LocalChannelUrlDirectory::registerChannelUrlsAsync(
        const std::string& channelId,
        types::ChannelUrlInformation channelUrlInformation,
        std::function<void(void)> onSuccess,
        std::function<void(const exceptions::JoynrException& error)> onError)
{
    LOG_INFO(logger, FormatString("registering Urls for id=%1").arg(channelId).str());
    return channelUrlDirectoryProxy->registerChannelUrlsAsync(
            channelId, channelUrlInformation, onSuccess, onError);
}

std::shared_ptr<joynr::Future<void>> LocalChannelUrlDirectory::unregisterChannelUrlsAsync(
        const std::string& channelId,
        std::function<void(void)> onSuccess,
        std::function<void(const exceptions::JoynrException& error)> onError)
{
    LOG_TRACE(logger, FormatString("unregistering ALL Urls for id=%1").arg(channelId).str());
    return channelUrlDirectoryProxy->unregisterChannelUrlsAsync(channelId, onSuccess, onError);
}

std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> LocalChannelUrlDirectory::
        getUrlsForChannelAsync(
                const std::string& channelId,
                const int64_t& timeout_ms,
                std::function<void(const types::ChannelUrlInformation& channelUrls)> onSuccess,
                std::function<void(const exceptions::JoynrException& error)> onError)
{
    QString channelIdQT = QString::fromStdString(channelId);
    LOG_TRACE(logger,
              FormatString("trying to getUrlsForChannel for id=%1")
                      .arg(channelIdQT.toStdString())
                      .str());

    if (localCache.contains(channelIdQT)) {
        LOG_TRACE(logger,
                  FormatString("using cached Urls for id=%1").arg(channelIdQT.toStdString()).str());
        std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> future(
                new joynr::Future<joynr::types::ChannelUrlInformation>());
        future->onSuccess(localCache.value(channelIdQT));
        if (onSuccess) {
            onSuccess(localCache.value(channelIdQT));
        }
        return future;
    }
    assert(channelUrlDirectoryProxy);
    std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> future(
            channelUrlDirectoryProxy->getUrlsForChannelAsync(channelId, onSuccess, onError));
    try {
        future->wait(timeout_ms);
        if (future->getStatus().successful()) {
            LOG_INFO(logger,
                     FormatString("Received remote url information for channelId=%1")
                             .arg(channelIdQT.toStdString())
                             .str());
            joynr::types::ChannelUrlInformation urls;
            future->get(urls);
            localCache.insert(channelIdQT, urls);
            LOG_INFO(logger,
                     FormatString("Stored url information for channelId=%1")
                             .arg(channelIdQT.toStdString())
                             .str());
        } else {
            LOG_INFO(logger,
                     FormatString("FAILED to receive remote url information for channelId=%1 . "
                                  "Status: %2")
                             .arg(channelIdQT.toStdString())
                             .arg(future->getStatus().toString())
                             .str());
        }
    } catch (joynr::exceptions::JoynrException& e) {
        // catches exceptions from both wait() and / or get() calls
        LOG_INFO(logger,
                 FormatString(
                         "FAILED to receive remote url information for channelId=%1 . Status: %2")
                         .arg(channelIdQT.toStdString())
                         .arg(e.getMessage())
                         .str());
    }
    return future;
}

} // namespace joynr
