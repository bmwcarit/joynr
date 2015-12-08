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
#include "joynr/PublicationManager.h"
#include "joynr/RequestCaller.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/JsonSerializer.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/MessagingQos.h"
#include "joynr/IPublicationSender.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/QtBroadcastFilterParameters.h"
#include "joynr/Util.h"
#include "joynr/IBroadcastFilter.h"
#include "libjoynr/subscription/SubscriptionRequestInformation.h"
#include "libjoynr/subscription/BroadcastSubscriptionRequestInformation.h"
#include "libjoynr/subscription/SubscriptionAttributeListener.h"
#include "libjoynr/subscription/SubscriptionBroadcastListener.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"

#include "joynr/SubscriptionUtil.h"

#include <QFile>
#include <cassert>
#include <chrono>
#include <stdint.h>
#include <mutex>

namespace joynr
{

using namespace std::chrono;

//------ HelperClasses ---------------------------------------------------------

class PublicationManager::Publication
{
public:
    Publication(IPublicationSender* publicationSender,
                std::shared_ptr<RequestCaller> requestCaller);
    ~Publication();

    qint64 timeOfLastPublication;
    IPublicationSender* sender;
    std::shared_ptr<RequestCaller> requestCaller;
    SubscriptionAttributeListener* attributeListener;
    SubscriptionBroadcastListener* broadcastListener;
    std::recursive_mutex mutex;
    DelayedScheduler::RunnableHandle publicationEndRunnableHandle;

private:
    DISALLOW_COPY_AND_ASSIGN(Publication);
};

class PublicationManager::PublisherRunnable : public Runnable
{
public:
    virtual ~PublisherRunnable();
    PublisherRunnable(PublicationManager& publicationManager, const QString& subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::pollSubscription()
    void run();

private:
    DISALLOW_COPY_AND_ASSIGN(PublisherRunnable);
    PublicationManager& publicationManager;
    QString subscriptionId;
};

class PublicationManager::PublicationEndRunnable : public Runnable
{
public:
    virtual ~PublicationEndRunnable();
    PublicationEndRunnable(PublicationManager& publicationManager, const QString& subscriptionId);

    void shutdown();

    // Calls PublicationManager::removePublication()
    void run();

private:
    DISALLOW_COPY_AND_ASSIGN(PublicationEndRunnable);
    PublicationManager& publicationManager;
    QString subscriptionId;
};

//------ PublicationManager ----------------------------------------------------

using namespace joynr_logging;
Logger* PublicationManager::logger = Logging::getInstance()->getLogger("MSG", "PublicationManager");

PublicationManager::~PublicationManager()
{
    LOG_DEBUG(logger, "Destructor, saving subscriptionsMap...");

    saveAttributeSubscriptionRequestsMap(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));
    saveBroadcastSubscriptionRequestsMap(
            subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));

    // saveSubscriptionRequestsMap will not store to file, as soon as shuttingDown is true, so we
    // call it first then set shuttingDown to true
    {
        std::lock_guard<std::mutex> shutDownLocker(shutDownMutex);
        shuttingDown = true;
    }

    LOG_DEBUG(logger, "Destructor, shutting down for thread pool and scheduler ...");
    delayedScheduler->shutdown();

    // Remove all publications
    LOG_DEBUG(logger, "Destructor: removing publications");

    while (subscriptionId2SubscriptionRequest.size() > 0) {
        auto subscriptionRequest = subscriptionId2SubscriptionRequest.begin();
        removeAttributePublication(
                QString::fromStdString((subscriptionRequest->second)->getSubscriptionId()));
    }

    while (subscriptionId2BroadcastSubscriptionRequest.size() > 0) {
        auto broadcastRequest = subscriptionId2BroadcastSubscriptionRequest.begin();
        removeBroadcastPublication(
                QString::fromStdString((broadcastRequest->second)->getSubscriptionId()));
    }
}

PublicationManager::PublicationManager(DelayedScheduler* scheduler)
        : publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          fileWriteLock(),
          delayedScheduler(scheduler),
          shutDownMutex(),
          shuttingDown(false),
          subscriptionRequestStorageFileName(QString::fromStdString(
                  LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME())),
          broadcastSubscriptionRequestStorageFileName(QString::fromStdString(
                  LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME())),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilters(),
          broadcastFilterLock()
{
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");
    qRegisterMetaType<std::shared_ptr<SubscriptionRequest>>("std::shared_ptr<SubscriptionRequest>");
    loadSavedAttributeSubscriptionRequestsMap();
    loadSavedBroadcastSubscriptionRequestsMap();
}

PublicationManager::PublicationManager(int maxThreads)
        : publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          fileWriteLock(),
          delayedScheduler(new ThreadPoolDelayedScheduler(maxThreads, "PubManager", 0)),
          shutDownMutex(),
          shuttingDown(false),
          subscriptionRequestStorageFileName(QString::fromStdString(
                  LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME())),
          broadcastSubscriptionRequestStorageFileName(QString::fromStdString(
                  LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME())),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilters(),
          broadcastFilterLock()
{
    loadSavedAttributeSubscriptionRequestsMap();
    loadSavedBroadcastSubscriptionRequestsMap();
}

bool isSubscriptionExpired(const SubscriptionQos* qos, int offset = 0)
{
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE() &&
           qos->getExpiryDate() < (now + offset);
}

void PublicationManager::add(const QString& proxyParticipantId,
                             const QString& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             SubscriptionRequest& subscriptionRequest,
                             IPublicationSender* publicationSender)
{
    assert(requestCaller);
    std::shared_ptr<SubscriptionRequestInformation> requestInfo(new SubscriptionRequestInformation(
            proxyParticipantId, providerParticipantId, subscriptionRequest));
    handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleAttributeSubscriptionRequest(
        std::shared_ptr<SubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        IPublicationSender* publicationSender)
{
    QString subscriptionId = QString::fromStdString(requestInfo->getSubscriptionId());
    std::shared_ptr<Publication> publication(new Publication(publicationSender, requestCaller));

    if (publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  "Publication with id: " +
                          QString::fromStdString(requestInfo->getSubscriptionId()) +
                          " already exists. Updating...");
        removeAttributePublication(subscriptionId);
    }

    subscriptionId2SubscriptionRequest.insert(subscriptionId.toStdString(), requestInfo);
    // Make note of the publication
    publications.insert(subscriptionId.toStdString(), publication);

    std::vector<Variant> subscriptionVector(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    LOG_DEBUG(logger, QString("added subscription: %1").arg(requestInfo->toQString()));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Add an onChange publication if needed
        addOnChangePublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const SubscriptionQos* qos = requestInfo->getSubscriptionQosPtr();
        int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int64_t publicationEndDelay = qos->getExpiryDate() - now;

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
                publication->publicationEndRunnableHandle = delayedScheduler->schedule(
                        new PublicationEndRunnable(*this, subscriptionId), publicationEndDelay);
                LOG_DEBUG(
                        logger, QString("publication will end in %1 ms").arg(publicationEndDelay));
            }
            {
                std::lock_guard<std::mutex> currentScheduledLocker(
                        currentScheduledPublicationsMutex);
                currentScheduledPublications.append(subscriptionId);
            }
            // sent at least once the current value
            delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId), -1);
        } else {
            LOG_WARN(logger, QString("publication end is in the past"));
        }
    }
    saveAttributeSubscriptionRequestsMap(subscriptionVector);
}

void PublicationManager::addOnChangePublication(
        const QString& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        LOG_TRACE(logger, QString("adding onChange subscription: %1").arg(subscriptionId));

        // Create an attribute listener to listen for onChange events
        SubscriptionAttributeListener* attributeListener =
                new SubscriptionAttributeListener(subscriptionId, *this);

        // Register the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->registerAttributeListener(request->getSubscribeToName(), attributeListener);

        // Make note of the attribute listener so that it can be unregistered
        publication->attributeListener = attributeListener;
    }
}

void PublicationManager::addBroadcastPublication(
        const QString& subscriptionId,
        std::shared_ptr<BroadcastSubscriptionRequestInformation> request,
        std::shared_ptr<PublicationManager::Publication> publication)
{
    LOG_TRACE(logger, QString("adding broadcast subscription: %1").arg(subscriptionId));

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));

    // Create a broadcast listener to listen for broadcast events
    SubscriptionBroadcastListener* broadcastListener =
            new SubscriptionBroadcastListener(subscriptionId, *this);

    // Register the broadcast listener
    std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
    requestCaller->registerBroadcastListener(request->getSubscribeToName(), broadcastListener);

    // Make note of the attribute listener so that it can be unregistered
    publication->broadcastListener = broadcastListener;
}

void PublicationManager::add(const QString& proxyParticipantId,
                             const QString& providerParticipantId,
                             SubscriptionRequest& subscriptionRequest)
{
    LOG_DEBUG(
            logger,
            "Added subscription for non existing provider (adding subscriptionRequest to queue).");
    std::shared_ptr<SubscriptionRequestInformation> requestInfo(new SubscriptionRequestInformation(
            proxyParticipantId, providerParticipantId, subscriptionRequest));
    {
        std::lock_guard<std::mutex> queueLocker(queuedSubscriptionRequestsMutex);
        queuedSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId().toStdString(), requestInfo));
    }

    subscriptionId2SubscriptionRequest.insert(requestInfo->getSubscriptionId(), requestInfo);
    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::add(const QString& proxyParticipantId,
                             const QString& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             BroadcastSubscriptionRequest& subscriptionRequest,
                             IPublicationSender* publicationSender)
{
    assert(requestCaller);
    std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
            new BroadcastSubscriptionRequestInformation(
                    proxyParticipantId, providerParticipantId, subscriptionRequest));

    handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleBroadcastSubscriptionRequest(
        std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        IPublicationSender* publicationSender)
{

    QString subscriptionId = QString::fromStdString(requestInfo->getSubscriptionId());

    std::shared_ptr<Publication> publication(new Publication(publicationSender, requestCaller));

    if (publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  "Publication with id: " +
                          QString::fromStdString(requestInfo->getSubscriptionId()) +
                          " already exists. Updating...");
        removeBroadcastPublication(subscriptionId);
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(subscriptionId.toStdString(), requestInfo);

    // Make note of the publication
    publications.insert(subscriptionId.toStdString(), publication);
    LOG_DEBUG(logger, QString("added subscription: %1").arg(requestInfo->toQString()));

    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Add an onChange publication if needed
        addBroadcastPublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const SubscriptionQos* qos = requestInfo->getSubscriptionQosPtr();
        int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int64_t publicationEndDelay = qos->getExpiryDate() - now;

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
                publication->publicationEndRunnableHandle = delayedScheduler->schedule(
                        new PublicationEndRunnable(*this, subscriptionId), publicationEndDelay);
                LOG_DEBUG(
                        logger, QString("publication will end in %1 ms").arg(publicationEndDelay));
            }
        } else {
            LOG_WARN(logger, QString("publication end is in the past"));
        }
    }
    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::add(const QString& proxyParticipantId,
                             const QString& providerParticipantId,
                             BroadcastSubscriptionRequest& subscriptionRequest)
{
    LOG_DEBUG(logger,
              "Added broadcast subscription for non existing provider (adding "
              "subscriptionRequest to queue).");
    std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
            new BroadcastSubscriptionRequestInformation(
                    proxyParticipantId, providerParticipantId, subscriptionRequest));
    {
        std::lock_guard<std::mutex> queueLocker(queuedBroadcastSubscriptionRequestsMutex);
        queuedBroadcastSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId().toStdString(), requestInfo));
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(
            requestInfo->getSubscriptionId(), requestInfo);
    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));

    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeAllSubscriptions(const QString& providerId)
{
    LOG_DEBUG(logger, QString("Removing all subscriptions for provider id= %1").arg(providerId));

    // Build lists of subscriptionIds to remove
    QString subscriptionId;

    QList<QString> publicationsToRemove;
    {
        for (auto& requestInfo : subscriptionId2SubscriptionRequest) {
            subscriptionId = QString::fromStdString((requestInfo.second)->getSubscriptionId());

            if ((requestInfo.second)->getProviderId() == providerId) {
                publicationsToRemove.append(subscriptionId);
            }
        }
    }

    QList<QString> broadcastsToRemove;
    {
        for (auto& requestInfo : subscriptionId2BroadcastSubscriptionRequest) {
            subscriptionId = QString::fromStdString((requestInfo.second)->getSubscriptionId());

            if ((requestInfo.second)->getProviderId() == providerId) {
                broadcastsToRemove.append(subscriptionId);
            }
        }
    }

    // Remove each publication
    for (const QString& subscriptionId : publicationsToRemove) {
        LOG_DEBUG(logger,
                  QString("Removing subscription providerId= %1, subscriptionId =%2")
                          .arg(providerId)
                          .arg(subscriptionId));
        removeAttributePublication(subscriptionId);
    }

    // Remove each broadcast
    for (const QString& subscriptionId : broadcastsToRemove) {
        LOG_DEBUG(logger,
                  QString("Removing subscription providerId= %1, subscriptionId =%2")
                          .arg(providerId)
                          .arg(subscriptionId));
        removeBroadcastPublication(subscriptionId);
    }
}

void PublicationManager::stopPublication(const QString& subscriptionId)
{
    LOG_DEBUG(logger, QString("stopPublication: %1").arg(subscriptionId));
    removePublication(subscriptionId);
}

bool PublicationManager::publicationExists(const QString& subscriptionId)
{
    return publications.contains(subscriptionId.toStdString());
}

void PublicationManager::restore(const QString& providerId,
                                 std::shared_ptr<RequestCaller> requestCaller,
                                 IPublicationSender* publicationSender)
{
    LOG_DEBUG(logger, "restore: entering ...");

    {
        std::lock_guard<std::mutex> queueLocker(queuedSubscriptionRequestsMutex);
        std::multimap<std::string, std::shared_ptr<SubscriptionRequestInformation>>::iterator
                queuedSubscriptionRequestsIterator =
                        queuedSubscriptionRequests.find(providerId.toStdString());
        while (queuedSubscriptionRequestsIterator != queuedSubscriptionRequests.end()) {
            std::shared_ptr<SubscriptionRequestInformation> requestInfo(
                    queuedSubscriptionRequestsIterator->second);
            queuedSubscriptionRequests.erase(queuedSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
                LOG_DEBUG(logger,
                          QString("Restoring subscription for provider: %1 %2").arg(providerId).arg(
                                  requestInfo->toQString()));
                handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedSubscriptionRequestsIterator =
                    queuedSubscriptionRequests.find(providerId.toStdString());
        }
    }

    {
        std::lock_guard<std::mutex> queueLocker(queuedBroadcastSubscriptionRequestsMutex);
        std::multimap<std::string,
                      std::shared_ptr<BroadcastSubscriptionRequestInformation>>::iterator
                queuedBroadcastSubscriptionRequestsIterator =
                        queuedBroadcastSubscriptionRequests.find(providerId.toStdString());
        while (queuedBroadcastSubscriptionRequestsIterator !=
               queuedBroadcastSubscriptionRequests.end()) {
            std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
                    queuedBroadcastSubscriptionRequestsIterator->second);
            queuedBroadcastSubscriptionRequests.erase(queuedBroadcastSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
                LOG_DEBUG(logger,
                          QString("Restoring subscription for provider: %1 %2").arg(providerId).arg(
                                  requestInfo->toQString()));
                handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedBroadcastSubscriptionRequestsIterator =
                    queuedBroadcastSubscriptionRequests.find(providerId.toStdString());
        }
    }
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveAttributeSubscriptionRequestsMap(
        const std::vector<Variant>& subscriptionVector)
{
    LOG_DEBUG(logger, "Saving active attribute subscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionVector, subscriptionRequestStorageFileName);
}

void PublicationManager::loadSavedAttributeSubscriptionRequestsMap()
{

    loadSavedSubscriptionRequestsMap<SubscriptionRequestInformation>(
            subscriptionRequestStorageFileName,
            queuedSubscriptionRequestsMutex,
            queuedSubscriptionRequests);
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveBroadcastSubscriptionRequestsMap(
        const std::vector<Variant>& subscriptionVector)
{
    LOG_DEBUG(logger, "Saving active broadcastSubscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionVector, broadcastSubscriptionRequestStorageFileName);
}

void PublicationManager::loadSavedBroadcastSubscriptionRequestsMap()
{
    LOG_DEBUG(logger, "Loading stored BroadcastSubscriptionrequests.");

    loadSavedSubscriptionRequestsMap<BroadcastSubscriptionRequestInformation>(
            broadcastSubscriptionRequestStorageFileName,
            queuedBroadcastSubscriptionRequestsMutex,
            queuedBroadcastSubscriptionRequests);
}

// Returns a list containing copies of the values of map
template <class RequestInformationType>
QList<QVariant> PublicationManager::subscriptionMapToListCopy(
        const std::map<std::string, std::shared_ptr<RequestInformationType>>& map)
{
    QList<QVariant> subscriptionList;
    {
        for (std::shared_ptr<RequestInformationType> requestInfo : map) {
            if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
                subscriptionList.append(QVariant::fromValue(*requestInfo));
            }
        }
    }
    return subscriptionList;
}

template <class RequestInformationType>
std::vector<Variant> PublicationManager::subscriptionMapToVectorCopy(
        const ThreadSafeMap<std::string, std::shared_ptr<RequestInformationType>>& map)
{
    std::vector<Variant> subscriptionVector;
    {
        for (mapIterator<std::string, std::shared_ptr<RequestInformationType>> iterator =
                     map.begin();
             iterator != map.end();
             ++iterator) {
            if (!isSubscriptionExpired((iterator->second)->getSubscriptionQosPtr())) {
                subscriptionVector.push_back(
                        Variant::make<RequestInformationType>(*iterator->second.get()));
            }
        }
    }
    return subscriptionVector;
}

// This function assumes that subscriptionVector is a copy that is exclusively used by this function
void PublicationManager::saveSubscriptionRequestsMap(const std::vector<Variant>& subscriptionVector,
                                                     const QString& storageFilename)
{
    if (isShuttingDown()) {
        LOG_DEBUG(logger, "Abort saving, because we are already shutting down.");
        return;
    }

    {
        std::lock_guard<std::mutex> fileWritelocker(fileWriteLock);
        QFile file(storageFilename);
        if (!file.open(QIODevice::WriteOnly)) {
            LOG_ERROR(logger,
                      QString("Could not open subscription request storage file: %1")
                              .arg(file.errorString()));
            return;
        }

        // Write the subscription information as a json list
        file.resize(0);

        std::string json = JsonSerializer::serializeVector(subscriptionVector);
        file.write(json.c_str());
    }
}

template <class RequestInformationType>
void PublicationManager::loadSavedSubscriptionRequestsMap(
        const QString& storageFilename,
        std::mutex& queueMutex,
        std::multimap<std::string, std::shared_ptr<RequestInformationType>>& queuedSubscriptions)
{

    static_assert(std::is_base_of<SubscriptionRequest, RequestInformationType>::value,
                  "loadSavedSubscriptionRequestsMap can only be used for subclasses of "
                  "SubscriptionRequest");

    QFile file(storageFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(logger,
                  QString("Unable to read file: %1, reson: %2").arg(storageFilename).arg(
                          file.errorString()));
        return;
    }

    // Read the Json into memory
    QByteArray jsonBytes = file.readAll();
    LOG_DEBUG(logger, QString("jsonBytes: %1").arg(QString::fromUtf8(jsonBytes)));

    // Deserialize the JSON into a list of subscription requests
    std::vector<RequestInformationType*> subscriptionVector =
            JsonSerializer::deserializeVector<RequestInformationType>(
                    QString::fromUtf8(jsonBytes).toStdString());

    // Loop through the saved subscriptions
    std::lock_guard<std::mutex> queueLocker(queueMutex);

    while (!subscriptionVector.empty()) {
        std::shared_ptr<RequestInformationType> requestInfo(*(subscriptionVector.begin()));
        subscriptionVector.erase(subscriptionVector.begin());

        // Add the subscription if it is still valid
        if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
            QString providerId = requestInfo->getProviderId();
            queuedSubscriptions.insert(std::make_pair(providerId.toStdString(), requestInfo));
            LOG_DEBUG(logger,
                      QString("Queuing subscription Request: %1 : %2").arg(providerId).arg(
                              requestInfo->toQString()));
        }
    }
}

void PublicationManager::removeAttributePublication(const QString& subscriptionId)
{
    LOG_DEBUG(logger, QString("removePublication: %1").arg(subscriptionId));

    if (!publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  QString("publication %1 does not exist - will not remove").arg(subscriptionId));
        return;
    }

    std::shared_ptr<Publication> publication(publications.take(subscriptionId.toStdString()));
    std::shared_ptr<SubscriptionRequestInformation> request(
            subscriptionId2SubscriptionRequest.take(subscriptionId.toStdString()));

    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Delete the onChange publication if needed
        removeOnChangePublication(subscriptionId, request, publication);
    }

    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeBroadcastPublication(const QString& subscriptionId)
{
    LOG_DEBUG(logger, QString("removeBroadcast: %1").arg(subscriptionId));

    if (!publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  QString("publication %1 does not exist - will not remove").arg(subscriptionId));
        return;
    }

    std::shared_ptr<Publication> publication = nullptr;
    if (publications.contains(subscriptionId.toStdString())) {
        publication = std::shared_ptr<Publication>(publications.take(subscriptionId.toStdString()));
    }

    std::shared_ptr<BroadcastSubscriptionRequestInformation> request = nullptr;
    if (subscriptionId2BroadcastSubscriptionRequest.contains(subscriptionId.toStdString())) {
        request = std::shared_ptr<BroadcastSubscriptionRequestInformation>(
                subscriptionId2BroadcastSubscriptionRequest.take(subscriptionId.toStdString()));
    }

    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    if (publication != nullptr && request != nullptr) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Remove listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterBroadcastListener(
                request->getSubscribeToName(), publication->broadcastListener);
        publication->broadcastListener = NULL;

        removePublicationEndRunnable(publication);
    }

    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeOnChangePublication(
        const QString& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        LOG_DEBUG(logger, QString("Removing onChange publication for id = %1").arg(subscriptionId));

        // Unregister and delete the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterAttributeListener(
                request->getSubscribeToName(), publication->attributeListener);
        publication->attributeListener = NULL;
    }
    removePublicationEndRunnable(publication);
}

// This function assumes a write lock is alrady held for the publication}
void PublicationManager::removePublicationEndRunnable(std::shared_ptr<Publication> publication)
{
    if (publication->publicationEndRunnableHandle != DelayedScheduler::INVALID_RUNNABLE_HANDLE &&
        !isShuttingDown()) {
        LOG_DEBUG(logger,
                  QString("Unscheduling PublicationEndRunnable with handle: %1")
                          .arg(publication->publicationEndRunnableHandle));
        delayedScheduler->unschedule(publication->publicationEndRunnableHandle);
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

// This function assumes that a read lock is already held
bool PublicationManager::processFilterChain(const QString& subscriptionId,
                                            const std::vector<Variant>& broadcastValues,
                                            const QList<std::shared_ptr<IBroadcastFilter>>& filters)
{
    bool success = true;

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId.toStdString()));
    BroadcastFilterParameters filterParameters = subscriptionRequest->getFilterParameters();

    for (std::shared_ptr<IBroadcastFilter> filter : filters) {
        success = success && filter->filter(broadcastValues, filterParameters);
    }
    return success;
}

bool PublicationManager::isShuttingDown()
{
    std::lock_guard<std::mutex> shuwDownLocker(shutDownMutex);
    return shuttingDown;
}

int64_t PublicationManager::getPublicationTtl(
        std::shared_ptr<SubscriptionRequest> subscriptionRequest) const
{
    const SubscriptionQos* qosPtr = subscriptionRequest->getSubscriptionQosPtr();
    return qosPtr->getPublicationTtl();
}

void PublicationManager::sendPublicationError(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        const exceptions::JoynrException& exception)
{
    LOG_DEBUG(logger, "sending subscription error");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setError(exceptions::JoynrExceptionUtil::createVariant(exception));
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, subscriptionPublication);
    LOG_DEBUG(logger, "sent subscription error");
}

void PublicationManager::sendSubscriptionPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        SubscriptionPublication& subscriptionPublication)
{

    MessagingQos mQos;

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    // Set the TTL
    mQos.setTtl(getPublicationTtl(request));

    IPublicationSender* publicationSender = publication->sender;

    publicationSender->sendSubscriptionPublication(
            subscriptionInformation->getProviderId().toStdString(),
            subscriptionInformation->getProxyId().toStdString(),
            mQos,
            subscriptionPublication);

    // Make note of when this publication was sent
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    publication->timeOfLastPublication = now;

    {
        std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
        currentScheduledPublications.removeAll(
                QString::fromStdString(request->getSubscriptionId()));
    }
    LOG_TRACE(logger, QString("sent publication @ %1").arg(now));
}

void PublicationManager::sendPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        const std::vector<Variant>& value)
{
    LOG_DEBUG(logger, "sending subscription reply");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setResponse(value);
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, subscriptionPublication);
    LOG_TRACE(logger, QString("sent subscription reply"));
}

void PublicationManager::pollSubscription(const QString& subscriptionId)
{
    LOG_TRACE(logger, QString("pollSubscription %1").arg(subscriptionId));

    if (isShuttingDown() || !publicationExists(subscriptionId) ||
        !subscriptionId2SubscriptionRequest.contains(subscriptionId.toStdString())) {
        return;
    }

    // Get the subscription details
    std::shared_ptr<Publication> publication(publications.value(subscriptionId.toStdString()));
    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId.toStdString()));
    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // See if the publication is needed
        const SubscriptionQos* qos = subscriptionRequest->getSubscriptionQosPtr();
        int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int64_t publicationInterval =
                SubscriptionUtil::getPeriodicPublicationInterval(subscriptionRequest->getQos());

        // check if the subscription qos needs a periodic publication
        if (publicationInterval > 0) {
            qint64 timeSinceLast = now - publication->timeOfLastPublication;
            // publish only if not published in the current interval
            if (timeSinceLast < publicationInterval) {
                LOG_DEBUG(logger,
                          QString("no publication necessary. publicationInterval: %1, "
                                  "timeSinceLast %2")
                                  .arg(publicationInterval)
                                  .arg(timeSinceLast));

                qint64 delayUntilNextPublication = publicationInterval - timeSinceLast;
                assert(delayUntilNextPublication >= 0);
                delayedScheduler->schedule(
                        new PublisherRunnable(*this, subscriptionId), delayUntilNextPublication);
                return;
            }
        }

        // Get the value of the attribute
        QString attributeGetter(Util::attributeGetterFromName(
                QString::fromStdString(subscriptionRequest->getSubscribeToName())));
        std::shared_ptr<RequestCaller> requestCaller(publication->requestCaller);
        std::shared_ptr<IRequestInterpreter> requestInterpreter(
                InterfaceRegistrar::instance().getRequestInterpreter(
                        requestCaller->getInterfaceName()));

        std::function<void(const std::vector<Variant>&)> onSuccess =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const std::vector<Variant>& response) {
            sendPublication(publication, subscriptionRequest, subscriptionRequest, response);

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                LOG_DEBUG(logger,
                          QString("rescheduling runnable with delay: %1").arg(publicationInterval));
                delayedScheduler->schedule(
                        new PublisherRunnable(*this, subscriptionId), publicationInterval);
            }
        };

        std::function<void(const exceptions::JoynrException&)> onError =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const exceptions::JoynrException& exception) {

            sendPublicationError(publication, subscriptionRequest, subscriptionRequest, exception);

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                LOG_DEBUG(logger,
                          QString("rescheduling runnable with delay: %1").arg(publicationInterval));
                delayedScheduler->schedule(
                        new PublisherRunnable(*this, subscriptionId), publicationInterval);
            }
        };

        LOG_DEBUG(logger, QString("run: executing requestInterpreter= %1").arg(attributeGetter));
        try {
            requestInterpreter->execute(requestCaller,
                                        attributeGetter.toStdString(),
                                        std::vector<Variant>(),
                                        std::vector<std::string>(),
                                        onSuccess,
                                        onError);
            // ApplicationException is not possible for attributes in Franca
        } catch (exceptions::ProviderRuntimeException& e) {
            std::string message = "Could not perform pollSubscription, caught exception: " +
                                  e.getTypeName() + ":" + e.getMessage();
            LOG_ERROR(logger, message.c_str());
            onError(e);
        } catch (exceptions::JoynrRuntimeException& e) {
            std::string message = "Could not perform an pollSubscription, caught exception: " +
                                  e.getTypeName() + ":" + e.getMessage();
            LOG_ERROR(logger, message.c_str());
            onError(exceptions::ProviderRuntimeException("caught exception: " + e.getTypeName() +
                                                         ":" + e.getMessage()));
        }
    }
}

void PublicationManager::removePublication(const QString& subscriptionId)
{
    if (subscriptionId2SubscriptionRequest.contains(subscriptionId.toStdString())) {
        removeAttributePublication(subscriptionId);
    } else if (subscriptionId2BroadcastSubscriptionRequest.contains(subscriptionId.toStdString())) {
        removeBroadcastPublication(subscriptionId);
    }
}

void PublicationManager::attributeValueChanged(const QString& subscriptionId, const Variant& value)
{
    LOG_DEBUG(logger,
              QString("attributeValueChanged for onChange subscription %1").arg(subscriptionId));

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        LOG_ERROR(logger,
                  QString("attributeValueChanged called for non-existing subscription %1")
                          .arg(subscriptionId));
        return;
    }

    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId.toStdString()));

    std::shared_ptr<Publication> publication(publications.value(subscriptionId.toStdString()));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        if (!isPublicationAlreadyScheduled(subscriptionId)) {
            qint64 timeUntilNextPublication =
                    getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

            if (timeUntilNextPublication == 0) {
                // Send the publication
                std::vector<Variant> values;
                values.push_back(value);
                sendPublication(publication, subscriptionRequest, subscriptionRequest, values);
            } else {
                reschedulePublication(subscriptionId, timeUntilNextPublication);
            }
        }
    }
}

void PublicationManager::broadcastOccurred(const QString& subscriptionId,
                                           const std::vector<Variant>& values,
                                           const QList<std::shared_ptr<IBroadcastFilter>>& filters)
{
    LOG_DEBUG(logger,
              QString("broadcastOccurred for subscription %1. Number of values: %2")
                      .arg(subscriptionId)
                      .arg(values.size()));

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        LOG_ERROR(logger,
                  QString("broadcastOccurred called for non-existing subscription %1")
                          .arg(subscriptionId));
        return;
    }

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId.toStdString()));
    std::shared_ptr<Publication> publication(publications.value(subscriptionId.toStdString()));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Only proceed if publication can immediately be sent
        qint64 timeUntilNextPublication =
                getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

        if (timeUntilNextPublication == 0) {
            // Execute broadcast filters
            if (processFilterChain(subscriptionId, values, filters)) {
                // Send the publication
                sendPublication(publication, subscriptionRequest, subscriptionRequest, values);
            }
        } else {
            LOG_DEBUG(
                    logger,
                    QString("Omitting broadcast publication for subscription %1 because of ")
                            .append(timeUntilNextPublication > 0 ? "too short interval. Next "
                                                                   "publication possible in %2 ms."
                                                                 : " error.")
                            .arg(subscriptionId)
                            .arg(timeUntilNextPublication));
        }
    }
}

bool PublicationManager::isPublicationAlreadyScheduled(const QString& subscriptionId)
{
    std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
    return currentScheduledPublications.contains(subscriptionId);
}

int64_t PublicationManager::getTimeUntilNextPublication(std::shared_ptr<Publication> publication,
                                                        Variant qos)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    // Check the last publication time against the min interval
    int64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    int64_t minInterval = SubscriptionUtil::getMinInterval(qos);

    qint64 timeSinceLast = now - publication->timeOfLastPublication;

    if (minInterval > 0 && timeSinceLast < minInterval) {
        return minInterval - timeSinceLast;
    }

    return 0;
}

void PublicationManager::reschedulePublication(const QString& subscriptionId,
                                               qint64 nextPublication)
{
    if (nextPublication > 0) {
        std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);

        // Schedule a publication so that the change is not forgotten
        if (!currentScheduledPublications.contains(subscriptionId)) {
            LOG_DEBUG(logger, QString("rescheduling runnable with delay: %1").arg(nextPublication));
            currentScheduledPublications.append(subscriptionId);
            delayedScheduler->schedule(
                    new PublisherRunnable(*this, subscriptionId), nextPublication);
        }
    }
}

//------ PublicationManager::Publication ---------------------------------------

PublicationManager::Publication::~Publication()
{
    // This class is not responsible for deleting the PublicationSender or AttributeListener
}

PublicationManager::Publication::Publication(IPublicationSender* publicationSender,
                                             std::shared_ptr<RequestCaller> requestCaller)
        : timeOfLastPublication(0),
          sender(publicationSender),
          requestCaller(requestCaller),
          attributeListener(NULL),
          broadcastListener(NULL),
          mutex(),
          publicationEndRunnableHandle(DelayedScheduler::INVALID_RUNNABLE_HANDLE)
{
}

//------ PublicationManager::PublisherRunnable ---------------------------------

PublicationManager::PublisherRunnable::~PublisherRunnable()
{
}

PublicationManager::PublisherRunnable::PublisherRunnable(PublicationManager& publicationManager,
                                                         const QString& subscriptionId)
        : Runnable(true), publicationManager(publicationManager), subscriptionId(subscriptionId)
{
}

void PublicationManager::PublisherRunnable::shutdown()
{
}

void PublicationManager::PublisherRunnable::run()
{
    publicationManager.pollSubscription(subscriptionId);
}

//------ PublicationManager::PublicationEndRunnable ----------------------------

PublicationManager::PublicationEndRunnable::~PublicationEndRunnable()
{
}

PublicationManager::PublicationEndRunnable::PublicationEndRunnable(
        PublicationManager& publicationManager,
        const QString& subscriptionId)
        : Runnable(true), publicationManager(publicationManager), subscriptionId(subscriptionId)
{
}

void PublicationManager::PublicationEndRunnable::shutdown()
{
}

void PublicationManager::PublicationEndRunnable::run()
{
    if (!publicationManager.publicationExists(subscriptionId)) {
        return;
    }
    std::shared_ptr<Publication> publication(
            publicationManager.publications.value(subscriptionId.toStdString()));
    publicationManager.removePublication(subscriptionId);

    {
        std::lock_guard<std::recursive_mutex> lock((publication->mutex));
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

} // namespace joynr
