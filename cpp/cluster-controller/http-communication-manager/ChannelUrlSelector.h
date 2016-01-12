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
#ifndef CHANNELDIRECTORYURLCACHE_H_
#define CHANNELDIRECTORYURLCACHE_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/Logger.h"
#include "cluster-controller/http-communication-manager/IChannelUrlSelector.h"
#include "joynr/types/ChannelUrlInformation.h"
#include "joynr/BounceProxyUrl.h"
#include <stdint.h>
#include <memory>
#include <chrono>

#include <QMap>

// Forward declare test classes
class ChannelUrlSelectorTest_punishTest_Test;
class ChannelUrlSelectorTest_updateTest_Test;
class ChannelUrlSelectorTest_initFittnessTest_Test;

namespace joynr
{

class ILocalChannelUrlDirectory;
class ChannelUrlSelectorEntry;

/**
 * @brief ChannelUrlSelector
 * Used by the MessageSender to obtain the 'best' Url available for a channelId. The 'best' is
 * determined using feedback from former trials. The available Urls for a channelId are ranked
 *according to
 * their position, the first Url is ranked highest. Every Url is assigned a 'fitness' value.
 * This fitness is initialized to the rank of the Url. It cannot be higher than the rank of the
 * corrsponding Url. If a connection using an Url fails, its fitness value is reduced by
 *'punishMent' factor.
 * The 'best' Url is the Url with the highest fitness value. After 'timeForOneRecouperation'
 * has passed, the fitness value of all Urls are increased.
 *
 */
class JOYNRCLUSTERCONTROLLER_EXPORT ChannelUrlSelector : public IChannelUrlSelector
{

public:
    static std::chrono::milliseconds TIME_FOR_ONE_RECOUPERATION();
    static const double& PUNISHMENT_FACTOR();
    /**
     * @brief Initialize
     *
     * @param bounceProxyUrl
     * @param timeForOneRecouperation
     * @param punishmentFactor
     */
    explicit ChannelUrlSelector(const BounceProxyUrl& bounceProxyUrl,
                                std::chrono::milliseconds timeForOneRecouperation,
                                double punishmentFactor);

    ~ChannelUrlSelector() override;

    /**
    * @brief Uses the ChannelUrlDirectoryProxy to query the remote ChannelUrlDirectory
    *
    * @param channelUrlDirectoryProxy
    */
    void init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
              const MessagingSettings& settings) override;

    /**
    * @brief Get the "best" URL for this channel. Feedback is used to figure out which
    * URL is currently best depending on recent availability and initial ordering (eg direct before
    * bounceproxy URL.
    *
    * @param channelId
    * @param status
    * @param timeout
    * @return std::string
    */
    std::string obtainUrl(const std::string& channelId,
                          RequestStatus& status,
                          std::chrono::milliseconds timeout) override;
    /**
    * @brief Provide feedback on performance of URL: was the connection successful or not?
    *
    * @param success
    * @param channelId
    * @param url
    */
    void feedback(bool success, const std::string& channelId, std::string url) override;

private:
    DISALLOW_COPY_AND_ASSIGN(ChannelUrlSelector);
    std::string constructDefaultUrl(const std::string& channelId);
    std::string constructUrl(const std::string& baseUrl);
    std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory;
    const BounceProxyUrl& bounceProxyUrl;
    QMap<std::string, ChannelUrlSelectorEntry*> entries;
    std::chrono::milliseconds timeForOneRecouperation;
    double punishmentFactor;
    std::string channelUrlDirectoryUrl;
    bool useDefaultUrl;
    ADD_LOGGER(ChannelUrlSelector);
};

/**
 * @brief ChannelUrlSelectorEntry
 *
 * This is a "private Class" of ChannelUrlSelector. In order to use it with googleTest it has been
 *moved out of ChannelUrlSelector
 * Class, but stays within the same file, as noone else should use ChannelUrlSelector.
 *
 */
class JOYNRCLUSTERCONTROLLER_EXPORT ChannelUrlSelectorEntry
{
public:
    ChannelUrlSelectorEntry(const types::ChannelUrlInformation& urlInformation,
                            double punishmentFactor,
                            std::chrono::milliseconds timeForOneRecouperation);
    ~ChannelUrlSelectorEntry();
    /**
     * @brief Returns the Url with the higest fitness value.
     *
     * @return std::string
     */
    std::string best();
    /**
     * @brief Reduces the fitness value of Url url.
     *
     * @param url
     */
    void punish(const std::string& url);
    /**
     * @brief Initializes the fitness values, ranks the Urls according to their position
     * (first Url has highest rank).
     *
     */
    void initFitness();
    /**
     * @brief Checks whether time for one recouperation has passed and increases fitness values if
     *so.
     *
     */
    void updateFitness();
    /**
     * @brief Returns the current fitness values.
     *
     */
    std::vector<double> getFitness();

private:
    DISALLOW_COPY_AND_ASSIGN(ChannelUrlSelectorEntry);

    friend class ::ChannelUrlSelectorTest_punishTest_Test;
    friend class ::ChannelUrlSelectorTest_updateTest_Test;
    friend class ::ChannelUrlSelectorTest_initFittnessTest_Test;

    std::chrono::time_point<std::chrono::system_clock> lastUpdate;
    std::vector<double> fitness;
    types::ChannelUrlInformation urlInformation;
    double punishmentFactor;
    std::chrono::milliseconds timeForOneRecouperation;
    ADD_LOGGER(ChannelUrlSelectorEntry);
};

} // namespace joynr
#endif // CHANNELDIRECTORYURLCACHE_H_
