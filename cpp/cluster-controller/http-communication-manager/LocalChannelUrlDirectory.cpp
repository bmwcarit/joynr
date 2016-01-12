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
#include <QString>

namespace joynr
{

INIT_LOGGER(LocalChannelUrlDirectory);

LocalChannelUrlDirectory::LocalChannelUrlDirectory(
        MessagingSettings& messagingSettings,
        std::shared_ptr<infrastructure::ChannelUrlDirectoryProxy> channelUrlDirectoryProxy)
        : messagingSettings(messagingSettings),
          channelUrlDirectoryProxy(channelUrlDirectoryProxy),
          localCache()
{
    init();
    JOYNR_LOG_TRACE(logger) << "Created ...";
}

LocalChannelUrlDirectory::~LocalChannelUrlDirectory()
{
    JOYNR_LOG_TRACE(logger) << "Destroyed ...";
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
    JOYNR_LOG_TRACE(logger) << "Provisioned Global Channel URL Directory URL ("
                            << channelUrlDirectoryUrl << ") into Local "
                                                         "Channel URL Directory";

    // provisioning of Global Capabilities Directory URL
    types::ChannelUrlInformation capabilitiesDirectoryUrlInformation;
    std::vector<std::string> capabilitiesDirectoryUrls;
    std::string capabilitiesDirectoryUrl = messagingSettings.getCapabilitiesDirectoryUrl();
    capabilitiesDirectoryUrls.push_back(capabilitiesDirectoryUrl);
    capabilitiesDirectoryUrlInformation.setUrls(capabilitiesDirectoryUrls);
    localCache.insert(QtTypeUtil::toQt(messagingSettings.getCapabilitiesDirectoryChannelId()),
                      capabilitiesDirectoryUrlInformation);
    JOYNR_LOG_TRACE(logger) << "Provisioned Global Capabilities Directory URL ("
                            << capabilitiesDirectoryUrl << ") into Local "
                                                           "Channel URL Directory";
}

std::shared_ptr<joynr::Future<void>> LocalChannelUrlDirectory::registerChannelUrlsAsync(
        const std::string& channelId,
        types::ChannelUrlInformation channelUrlInformation,
        std::function<void(void)> onSuccess,
        std::function<void(const exceptions::JoynrException& error)> onError)
{
    JOYNR_LOG_INFO(logger) << "registering Urls for id=" << channelId;
    return channelUrlDirectoryProxy->registerChannelUrlsAsync(
            channelId, channelUrlInformation, onSuccess, onError);
}

std::shared_ptr<joynr::Future<void>> LocalChannelUrlDirectory::unregisterChannelUrlsAsync(
        const std::string& channelId,
        std::function<void(void)> onSuccess,
        std::function<void(const exceptions::JoynrException& error)> onError)
{
    JOYNR_LOG_TRACE(logger) << "unregistering ALL Urls for id=" << channelId;
    return channelUrlDirectoryProxy->unregisterChannelUrlsAsync(channelId, onSuccess, onError);
}

std::shared_ptr<joynr::Future<joynr::types::ChannelUrlInformation>> LocalChannelUrlDirectory::
        getUrlsForChannelAsync(
                const std::string& channelId,
                std::chrono::milliseconds timeout,
                std::function<void(const types::ChannelUrlInformation& channelUrls)> onSuccess,
                std::function<void(const exceptions::JoynrException& error)> onError)
{
    QString channelIdQT = QString::fromStdString(channelId);
    JOYNR_LOG_TRACE(logger) << "trying to getUrlsForChannel for id = " << channelIdQT.toStdString();

    if (localCache.contains(channelIdQT)) {
        JOYNR_LOG_TRACE(logger) << "using cached Urls for id = " << channelIdQT.toStdString();
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
        future->wait(timeout.count());
        if (future->getStatus().successful()) {
            JOYNR_LOG_INFO(logger) << "Received remote url information for channelId = "
                                   << channelIdQT.toStdString();
            joynr::types::ChannelUrlInformation urls;
            future->get(urls);
            localCache.insert(channelIdQT, urls);
            JOYNR_LOG_INFO(logger)
                    << "Stored url information for channelId = " << channelIdQT.toStdString();
        } else {
            JOYNR_LOG_INFO(logger) << "FAILED to receive remote url information for channelId = "
                                   << channelIdQT.toStdString() << ". "
                                   << "Status: " << future->getStatus().toString();
        }
    } catch (joynr::exceptions::JoynrException& e) {
        // catches exceptions from both wait() and / or get() calls
        JOYNR_LOG_INFO(logger) << "FAILED to receive remote url information for channelId = "
                               << channelIdQT.toStdString() << ". Status: " << e.getMessage();
    }
    return future;
}

} // namespace joynr
