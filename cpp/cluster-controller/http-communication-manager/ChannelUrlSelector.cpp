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
#include "cluster-controller/http-communication-manager/ChannelUrlSelector.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Future.h"

#include <QDateTime>
#include <cmath>
#include <memory>

namespace joynr
{

joynr_logging::Logger* ChannelUrlSelector::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "ChannelUrlSelector");

const double& ChannelUrlSelector::PUNISHMENT_FACTOR()
{
    static const double value = 0.4;
    return value;
}
const qint64& ChannelUrlSelector::TIME_FOR_ONE_RECOUPERATION()
{
    static const qint64 value = 1000 * 60 * 3;
    return value;
}

ChannelUrlSelector::ChannelUrlSelector(const BounceProxyUrl& bounceProxyUrl,
                                       qint64 timeForOneRecouperation,
                                       double punishmentFactor)
        : channelUrlDirectory(),
          bounceProxyUrl(bounceProxyUrl),
          entries(),
          timeForOneRecouperation(timeForOneRecouperation),
          punishmentFactor(punishmentFactor),
          channelUrlDirectoryUrl(),
          useDefaultUrl(true)
{
    LOG_TRACE(logger, "Created ...");
}

ChannelUrlSelector::~ChannelUrlSelector()
{
    // Delete all entries
    QMapIterator<QString, ChannelUrlSelectorEntry*> i(entries);
    ChannelUrlSelectorEntry* v;
    while (i.hasNext()) {
        i.next();
        v = i.value();
        if (v)
            delete v;
    }
    LOG_TRACE(logger, "Destroyed ...");
}

void ChannelUrlSelector::init(QSharedPointer<ILocalChannelUrlDirectory> channelUrlDirectory,
                              const MessagingSettings& settings)
{

    this->channelUrlDirectory = channelUrlDirectory;
    channelUrlDirectoryUrl = settings.getChannelUrlDirectoryUrl();
}

QString ChannelUrlSelector::obtainUrl(const QString& channelId,
                                      RequestStatus& status,
                                      const qint64& timeout_ms)
{

    LOG_TRACE(logger, "entering obtainUrl ...");
    status.setCode(RequestStatusCode::IN_PROGRESS);

    QString url("");

    if (channelUrlDirectory.isNull()) {
        LOG_DEBUG(logger, "obtainUrl: channelUrlDirectoryProxy not available ...");
        LOG_DEBUG(logger,
                  "using default url constructed from channelId and BounceproxyUrl instead...");
        status.setCode(RequestStatusCode::ERROR);
        url = constructDefaultUrl(channelId);
        return constructUrl(url);
    }

    if (entries.contains(channelId)) {
        LOG_DEBUG(logger, "obtainUrl: using cached Urls for id = " + channelId);
        ChannelUrlSelectorEntry* entry = entries.value(channelId);
        status.setCode(RequestStatusCode::OK);
        return constructUrl(entry->best());
    }
    LOG_DEBUG(logger,
              "obtainUrl: trying to obtain Urls from remote ChannelUrlDirectory for id = " +
                      channelId);
    std::shared_ptr<Future<types::ChannelUrlInformation>> proxyFuture(
            channelUrlDirectory->getUrlsForChannel(channelId.toStdString(), timeout_ms));
    status = proxyFuture->getStatus();

    if (status.successful()) {
        LOG_DEBUG(logger,
                  "obtainUrl: obtained Urls from remote ChannelUrlDirectory for id = " + channelId);
        types::ChannelUrlInformation urlInformation;
        proxyFuture->getValues(urlInformation);
        if (urlInformation.getUrls().empty()) {
            LOG_DEBUG(logger, "obtainUrl: empty list of urls obtained from id = " + channelId);
            LOG_DEBUG(logger, "obtainUrl: constructing default url for id = " + channelId);
            status.setCode(RequestStatusCode::ERROR);
            url = constructDefaultUrl(channelId);
            return constructUrl(url);
        }
        url = QString::fromStdString(urlInformation.getUrls().at(0)); // return the first, store all
        entries.insert(channelId,
                       new ChannelUrlSelectorEntry(
                               types::QtChannelUrlInformation::createQt(urlInformation),
                               punishmentFactor,
                               timeForOneRecouperation)); // deleted where?
        status.setCode(RequestStatusCode::OK);
        return constructUrl(url);
    } else {
        LOG_DEBUG(logger,
                  "obtainUrl: FAILED to obtain Urls from remote ChannelUrlDirectory for id = " +
                          channelId);
        status.setCode(RequestStatusCode::ERROR);
        url = constructDefaultUrl(channelId);
        return constructUrl(url);
    }
    return url;
}

void ChannelUrlSelector::feedback(bool success, const QString& channelId, QString url)
{

    LOG_TRACE(logger, "entering feedback ...");
    if (success) {
        LOG_TRACE(logger, "feedback was positive");
        return;
    }
    if (!entries.contains(channelId)) {
        LOG_DEBUG(logger, "feedback for an unknown channelId");
        return;
    }
    LOG_TRACE(logger, "feedback: punishing Url = " + url);
    LOG_TRACE(logger, " for channelId= " + channelId);
    ChannelUrlSelectorEntry* entry = entries.value(channelId);
    int cutoff = url.indexOf("/" + BounceProxyUrl::SEND_MESSAGE_PATH_APPENDIX());
    url.resize(cutoff);
    entry->punish(url);
    url.append("/"); // necessary because an Url can be provided without the ending /
    entry->punish(url);
}

QString ChannelUrlSelector::constructUrl(const QString& baseUrl)
{
    QUrl sendUrl(baseUrl);
    QString path = sendUrl.path();
    if (!path.endsWith("/")) {
        path.append("/");
    }
    path.append(BounceProxyUrl::SEND_MESSAGE_PATH_APPENDIX());
    path.append("/");
    sendUrl.setPath(path);
    return sendUrl.toString();
}

// TODO: needs to be removed in future when directoy is working! OR: declare as default strategy
QString ChannelUrlSelector::constructDefaultUrl(const QString& channelId)
{
    LOG_DEBUG(
            logger,
            "constructDefaultUrl ... using default Url inferred from channelId and BounceProxyUrl");
    if (!useDefaultUrl)
        assert(false);
    QString url = bounceProxyUrl.getBounceProxyBaseUrl().toString() + channelId;
    types::QtChannelUrlInformation urlInformation;
    QList<QString> urls;
    urls << url;
    urlInformation.setUrls(urls);
    entries.insert(
            channelId,
            new ChannelUrlSelectorEntry(urlInformation, punishmentFactor, timeForOneRecouperation));
    return url;
}

/**
  * IMPLEMENTATION OF ChannelUrlSelectorEntry
  *
  * ONLY TO BE USED BY ChannelUrlSelector
  * would be private but then it is too difficult to test using googlemock!
  *
  */

joynr_logging::Logger* ChannelUrlSelectorEntry::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "ChannelUrlSelectorEntry");

ChannelUrlSelectorEntry::ChannelUrlSelectorEntry(
        const types::QtChannelUrlInformation& urlInformation,
        double punishmentFactor,
        qint64 timeForOneRecouperation)
        : lastUpdate(QDateTime::currentMSecsSinceEpoch()),
          fitness(),
          urlInformation(urlInformation),
          punishmentFactor(punishmentFactor),
          timeForOneRecouperation(timeForOneRecouperation)
{

    LOG_TRACE(logger, "Created ...");
    initFitness();
}

ChannelUrlSelectorEntry::~ChannelUrlSelectorEntry()
{
    LOG_TRACE(logger, "Destroyed ...");
}

QString ChannelUrlSelectorEntry::best()
{
    LOG_TRACE(logger, "best ...");
    updateFitness();
    QList<QString> urls = urlInformation.getUrls();
    double temp = fitness.first();
    int posOfMax = 0;

    for (int i = 0; i < urls.size(); i++) {
        if (temp < fitness.at(i)) {
            temp = fitness.at(i);
            posOfMax = i;
        }
    }
    return urls.at(posOfMax);
}

void ChannelUrlSelectorEntry::punish(const QString& url)
{
    LOG_TRACE(logger, "punish ...");
    QList<QString> urls = urlInformation.getUrls();
    if (!urls.contains(url)) {
        LOG_DEBUG(logger, "Url not contained in cache entry ...");
        return;
    }
    updateFitness();
    int urlPosition = urls.indexOf(url);
    double urlFitness = fitness.at(urlPosition);
    urlFitness -= punishmentFactor;
    fitness.replace(urlPosition, urlFitness);
}

void ChannelUrlSelectorEntry::initFitness()
{
    LOG_TRACE(logger, "initFitness ...");
    QList<QString> urls = urlInformation.getUrls();
    double rank = urls.size();
    for (int i = 0; i < urls.size(); i++) {
        fitness.append(rank);
        rank -= 1;
    }
}

void ChannelUrlSelectorEntry::updateFitness()
{
    LOG_TRACE(logger, "updateFitness ...");
    // Is it time to increase the fitness of all Urls? (counterbalances punishments, forget some
    // history)
    qint64 timeSinceLastUpdate = QDateTime::currentMSecsSinceEpoch() - lastUpdate;
    double numberOfIncreases = floor(((double)timeSinceLastUpdate / timeForOneRecouperation));
    if (numberOfIncreases < 1) {
        return;
    }
    QList<QString> urls = urlInformation.getUrls();
    double increase = numberOfIncreases * punishmentFactor;
    double urlFitness = 0;

    for (int i = 0; i < urls.size(); i++) {
        urlFitness = fitness.at(i);
        urlFitness += increase; // did the fitness increase above the allowed value?
        if (urlFitness >
            urls.size() - i) { // the fitness of an url of rank eg 2 cannot be hiher than 2
            urlFitness = urls.size() - i;
        }
        fitness.replace(i, urlFitness);
    }
    lastUpdate = QDateTime::currentMSecsSinceEpoch();
}

QList<double> ChannelUrlSelectorEntry::getFitness()
{
    return fitness;
}

} // namespace joynr
