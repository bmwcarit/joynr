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
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"

#include <cmath>
#include <memory>
#include <chrono>

#include <QUrl>
#include <boost/algorithm/string/predicate.hpp>

#include "joynr/MessagingSettings.h"
#include "joynr/Future.h"
#include "joynr/DispatcherUtils.h"

namespace joynr
{

INIT_LOGGER(ChannelUrlSelector);

const double& ChannelUrlSelector::PUNISHMENT_FACTOR()
{
    static const double value = 0.4;
    return value;
}

std::chrono::milliseconds ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION()
{
    return std::chrono::minutes(3);
}

ChannelUrlSelector::ChannelUrlSelector(const BrokerUrl& brokerUrl,
                                       std::chrono::milliseconds timeForOneRecouperation,
                                       double punishmentFactor)
        : channelUrlDirectory(),
          brokerUrl(brokerUrl),
          entries(),
          timeForOneRecouperation(timeForOneRecouperation),
          punishmentFactor(punishmentFactor),
          channelUrlDirectoryUrl(),
          useDefaultUrl(true)
{
    JOYNR_LOG_TRACE(logger, "Created ...");
}

ChannelUrlSelector::~ChannelUrlSelector()
{
    // Delete all entries
    auto it = entries.begin();
    ChannelUrlSelectorEntry* v;
    while (it != entries.end()) {
        v = it->second;
        if (v) {
            delete v;
        }
        ++it;
    }
    JOYNR_LOG_TRACE(logger, "Destroyed ...");
}

void ChannelUrlSelector::init(std::shared_ptr<ILocalChannelUrlDirectory> channelUrlDirectory,
                              const MessagingSettings& settings)
{

    this->channelUrlDirectory = channelUrlDirectory;
    channelUrlDirectoryUrl = settings.getChannelUrlDirectoryUrl();
}

std::string ChannelUrlSelector::obtainUrl(const std::string& channelId,
                                          StatusCodeEnum& status,
                                          std::chrono::milliseconds timeout)
{

    JOYNR_LOG_TRACE(logger, "entering obtainUrl ...");
    status = StatusCodeEnum::SUCCESS;

    std::string url("");

    if (!channelUrlDirectory) {
        JOYNR_LOG_DEBUG(logger, "obtainUrl: channelUrlDirectoryProxy not available ...");
        JOYNR_LOG_DEBUG(
                logger, "using default url constructed from channelId and BrokerUrl instead...");
        status = StatusCodeEnum::ERROR;
        url = constructDefaultUrl(channelId);
        return constructUrl(url);
    }

    auto entryIt = entries.find(channelId);
    if (entryIt != entries.cend()) {
        JOYNR_LOG_DEBUG(logger, "obtainUrl: using cached Urls for id = {}", channelId);
        ChannelUrlSelectorEntry* entry = entryIt->second;
        status = StatusCodeEnum::SUCCESS;
        return constructUrl(entry->best());
    }
    JOYNR_LOG_DEBUG(logger,
                    "obtainUrl: trying to obtain Urls from remote ChannelUrlDirectory for id = {}",
                    channelId);
    std::shared_ptr<Future<types::ChannelUrlInformation>> proxyFuture(
            channelUrlDirectory->getUrlsForChannelAsync(channelId, timeout));
    status = proxyFuture->getStatus();

    if (status == StatusCodeEnum::SUCCESS) {
        JOYNR_LOG_DEBUG(logger,
                        "obtainUrl: obtained Urls from remote ChannelUrlDirectory for id = {}",
                        channelId);
        types::ChannelUrlInformation urlInformation;
        proxyFuture->get(urlInformation);
        if (urlInformation.getUrls().empty()) {
            JOYNR_LOG_DEBUG(
                    logger, "obtainUrl: empty list of urls obtained from id = {}", channelId);
            JOYNR_LOG_DEBUG(logger, "obtainUrl: constructing default url for id = {}", channelId);
            status = StatusCodeEnum::ERROR;
            url = constructDefaultUrl(channelId);
            return constructUrl(url);
        }
        url = urlInformation.getUrls().at(0); // return the first, store all
        entries.insert(
                std::make_pair(channelId,
                               new ChannelUrlSelectorEntry(
                                       urlInformation, punishmentFactor, timeForOneRecouperation)));
        status = StatusCodeEnum::SUCCESS;
        return constructUrl(url);
    } else {
        JOYNR_LOG_DEBUG(logger,
                        "obtainUrl: FAILED to obtain Urls from remote ChannelUrlDirectory "
                        "for id = {}",
                        channelId);
        status = StatusCodeEnum::ERROR;
        url = constructDefaultUrl(channelId);
        return constructUrl(url);
    }
    return url;
}

void ChannelUrlSelector::feedback(bool success, const std::string& channelId, std::string url)
{
    JOYNR_LOG_TRACE(logger, "entering feedback ...");
    if (success) {
        JOYNR_LOG_TRACE(logger, "feedback was positive");
        return;
    }

    auto entryIt = entries.find(channelId);
    if (entryIt == entries.cend()) {
        JOYNR_LOG_DEBUG(logger, "feedback for an unknown channelId");
        return;
    }
    JOYNR_LOG_TRACE(logger, "feedback: punishing Url = {}", url);
    JOYNR_LOG_TRACE(logger, " for channelId= {}", channelId);
    ChannelUrlSelectorEntry* entry = entryIt->second;
    std::string::size_type cutoff = url.find("/" + BrokerUrl::SEND_MESSAGE_PATH_APPENDIX(), 0);
    url.resize(cutoff);
    entry->punish(url);
    url.append("/"); // necessary because an Url can be provided without the ending /
    entry->punish(url);
}

std::string ChannelUrlSelector::constructUrl(const std::string& baseUrl)
{
    QUrl sendUrl(QString::fromStdString(baseUrl));
    std::string path = sendUrl.path().toStdString();
    using boost::algorithm::ends_with;
    if (!path.empty() && !ends_with(path, "/")) {
        path.append("/");
    }
    path.append(BrokerUrl::SEND_MESSAGE_PATH_APPENDIX());
    path.append("/");
    sendUrl.setPath(QString::fromStdString(path));
    return sendUrl.toString().toStdString();
}

// TODO: needs to be removed in future when directoy is working! OR: declare as default strategy
std::string ChannelUrlSelector::constructDefaultUrl(const std::string& channelId)
{
    JOYNR_LOG_DEBUG(logger,
                    "constructDefaultUrl ... using default Url inferred from channelId and "
                    "BrokerUrl");
    assert(useDefaultUrl);
    std::string url = brokerUrl.getBrokerChannelsBaseUrl().toString() + channelId;
    types::ChannelUrlInformation urlInformation;
    std::vector<std::string> urls;
    urls.push_back(url);
    urlInformation.setUrls(urls);
    entries.insert(
            std::make_pair(channelId,
                           new ChannelUrlSelectorEntry(
                                   urlInformation, punishmentFactor, timeForOneRecouperation)));
    return url;
}

/**
  * IMPLEMENTATION OF ChannelUrlSelectorEntry
  *
  * ONLY TO BE USED BY ChannelUrlSelector
  * would be private but then it is too difficult to test using googlemock!
  *
  */

INIT_LOGGER(ChannelUrlSelectorEntry);

ChannelUrlSelectorEntry::ChannelUrlSelectorEntry(const types::ChannelUrlInformation& urlInformation,
                                                 double punishmentFactor,
                                                 std::chrono::milliseconds timeForOneRecouperation)
        : lastUpdate(std::chrono::system_clock::now()),
          fitness(),
          urlInformation(urlInformation),
          punishmentFactor(punishmentFactor),
          timeForOneRecouperation(timeForOneRecouperation)
{

    JOYNR_LOG_TRACE(logger, "Created ...");
    initFitness();
}

ChannelUrlSelectorEntry::~ChannelUrlSelectorEntry()
{
    JOYNR_LOG_TRACE(logger, "Destroyed ...");
}

std::string ChannelUrlSelectorEntry::best()
{
    JOYNR_LOG_TRACE(logger, "best ...");
    updateFitness();
    const std::vector<std::string>& urls = urlInformation.getUrls();
    double temp = fitness[0];
    int posOfMax = 0;

    for (std::size_t i = 0; i < urls.size(); i++) {
        if (temp < fitness.at(i)) {
            temp = fitness.at(i);
            posOfMax = i;
        }
    }
    return urls.at(posOfMax);
}

void ChannelUrlSelectorEntry::punish(const std::string& url)
{
    JOYNR_LOG_TRACE(logger, "punish ...");
    const std::string stdUrl = url;
    const std::vector<std::string>& urls = urlInformation.getUrls();
    if (!util::vectorContains(urls, stdUrl)) {
        JOYNR_LOG_DEBUG(logger, "Url not contained in cache entry ...");
        return;
    }
    updateFitness();
    auto urlIt = std::find(urls.cbegin(), urls.cend(), stdUrl);
    std::size_t urlPosition = std::distance(urls.cbegin(), urlIt);
    double urlFitness = fitness.at(urlPosition);
    urlFitness -= punishmentFactor;
    fitness[urlPosition] = urlFitness;
}

void ChannelUrlSelectorEntry::initFitness()
{
    JOYNR_LOG_TRACE(logger, "initFitness ...");
    const std::vector<std::string>& urls = urlInformation.getUrls();
    double rank = urls.size();
    for (std::size_t i = 0; i < urls.size(); i++) {
        fitness.push_back(rank);
        rank -= 1;
    }
}

void ChannelUrlSelectorEntry::updateFitness()
{
    JOYNR_LOG_TRACE(logger, "updateFitness ...");
    // Is it time to increase the fitness of all Urls? (counterbalances punishments, forget some
    // history)
    std::chrono::milliseconds timeSinceLastUpdate =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() -
                                                                  lastUpdate);
    double numberOfIncreases =
            floor(((double)timeSinceLastUpdate.count() / timeForOneRecouperation.count()));
    if (numberOfIncreases < 1) {
        return;
    }
    std::vector<std::string> urls = urlInformation.getUrls();
    double increase = numberOfIncreases * punishmentFactor;
    double urlFitness = 0;

    for (std::size_t i = 0; i < urls.size(); i++) {
        urlFitness = fitness.at(i);
        urlFitness += increase; // did the fitness increase above the allowed value?
        if (urlFitness >
            urls.size() - i) { // the fitness of an url of rank eg 2 cannot be hiher than 2
            urlFitness = urls.size() - i;
        }
        fitness[i] = urlFitness;
    }
    lastUpdate = std::chrono::system_clock::now();
}

std::vector<double> ChannelUrlSelectorEntry::getFitness()
{
    return fitness;
}

} // namespace joynr
