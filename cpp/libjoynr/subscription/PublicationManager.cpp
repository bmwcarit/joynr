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
#include "joynr/PublicationManager.h"
#include "joynr/RequestCaller.h"
#include "joynr/DispatcherUtils.h"
#include "joynr/JsonSerializer.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/SubscriptionPublication.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/MessagingQos.h"
#include "joynr/IPublicationSender.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/BroadcastFilterParameters.h"
#include "joynr/Util.h"
#include "joynr/IBroadcastFilter.h"
#include "libjoynr/subscription/SubscriptionRequestInformation.h"
#include "libjoynr/subscription/BroadcastSubscriptionRequestInformation.h"
#include "libjoynr/subscription/SubscriptionAttributeListener.h"
#include "libjoynr/subscription/SubscriptionBroadcastListener.h"
#include "joynr/LibjoynrSettings.h"

#include "joynr/SubscriptionUtil.h"

#include <QFile>
#include <cassert>

namespace joynr
{

//------ HelperClasses ---------------------------------------------------------

class PublicationManager::Publication
{
public:
    Publication(IPublicationSender* publicationSender, QSharedPointer<RequestCaller> requestCaller);
    ~Publication();

    qint64 timeOfLastPublication;
    IPublicationSender* sender;
    QSharedPointer<RequestCaller> requestCaller;
    SubscriptionAttributeListener* attributeListener;
    SubscriptionBroadcastListener* broadcastListener;
    QMutex mutex;
    quint32 publicationEndRunnableHandle;

private:
    DISALLOW_COPY_AND_ASSIGN(Publication);
};

class PublicationManager::PublisherRunnable : public QRunnable
{
public:
    virtual ~PublisherRunnable();
    PublisherRunnable(PublicationManager& publicationManager, const QString& subscriptionId);

    // Calls PublicationManager::pollSubscription()
    void run();

private:
    DISALLOW_COPY_AND_ASSIGN(PublisherRunnable);
    PublicationManager& publicationManager;
    QString subscriptionId;
};

class PublicationManager::PublicationEndRunnable : public QRunnable
{
public:
    virtual ~PublicationEndRunnable();
    PublicationEndRunnable(PublicationManager& publicationManager, const QString& subscriptionId);

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
    QWriteLocker subscriptionLocker(&subscriptionLock);

    saveAttributeSubscriptionRequestsMap(
            subscriptionMapToListCopy(subscriptionId2SubscriptionRequest));
    saveBroadcastSubscriptionRequestsMap(
            subscriptionMapToListCopy(subscriptionId2BroadcastSubscriptionRequest));

    // saveSubscriptionRequestsMap will not store to file, as soon as shuttingDown is true, so we
    // call it first then set shuttingDown to true
    {
        QMutexLocker shutDownLocker(&shutDownMutex);
        shuttingDown = true;
    }

    subscriptionLocker.unlock();

    LOG_DEBUG(logger, "Destructor, waiting for thread pool ...");
    publishingThreadPool.waitForDone();

    LOG_DEBUG(logger, "Destructor, deleting scheduler ...");
    delete delayedScheduler;

    subscriptionLocker.relock();

    // Remove all publications
    LOG_DEBUG(logger, "Destructor: removing publications");
    foreach (QSharedPointer<SubscriptionRequestInformation> request,
             subscriptionId2SubscriptionRequest) {
        removeAttributePublication(request->getSubscriptionId());
    }
    foreach (QSharedPointer<BroadcastSubscriptionRequestInformation> request,
             subscriptionId2BroadcastSubscriptionRequest) {
        removeBroadcastPublication(request->getSubscriptionId());
    }

    subscriptionLocker.unlock();
}

PublicationManager::PublicationManager(DelayedScheduler* scheduler, int maxThreads)
        : publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          subscriptionLock(QReadWriteLock::Recursive),
          fileWriteLock(),
          publishingThreadPool(),
          delayedScheduler(scheduler),
          shutDownMutex(),
          shuttingDown(false),
          subscriptionRequestStorageFileName(
                  LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()),
          broadcastSubscriptionRequestStorageFileName(
                  LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME()),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilters(),
          broadcastFilterLock()
{

    publishingThreadPool.setMaxThreadCount(maxThreads);
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");
    qRegisterMetaType<QSharedPointer<SubscriptionRequest>>("QSharedPointer<SubscriptionRequest>");
    loadSavedAttributeSubscriptionRequestsMap();
    loadSavedBroadcastSubscriptionRequestsMap();
}

PublicationManager::PublicationManager(int maxThreads)
        : publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          subscriptionLock(QReadWriteLock::Recursive),
          fileWriteLock(),
          publishingThreadPool(),
          delayedScheduler(NULL),
          shutDownMutex(),
          shuttingDown(false),
          subscriptionRequestStorageFileName(
                  LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_STORAGE_FILENAME()),
          broadcastSubscriptionRequestStorageFileName(
                  LibjoynrSettings::DEFAULT_BROADCASTSUBSCRIPTIONREQUEST_STORAGE_FILENAME()),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilters(),
          broadcastFilterLock()
{

    publishingThreadPool.setMaxThreadCount(maxThreads);
    delayedScheduler = new ThreadPoolDelayedScheduler(
            publishingThreadPool, QString("PublicationManager-PublishingThreadPool"));
    qRegisterMetaType<SubscriptionRequest>("SubscriptionRequest");
    qRegisterMetaType<QSharedPointer<SubscriptionRequest>>("QSharedPointer<SubscriptionRequest>");
    loadSavedAttributeSubscriptionRequestsMap();
    loadSavedBroadcastSubscriptionRequestsMap();
}

bool isSubscriptionExpired(QSharedPointer<SubscriptionQos> qos, int offset = 0)
{
    return qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE() &&
           qos->getExpiryDate() < (QDateTime::currentMSecsSinceEpoch() + offset);
}

void PublicationManager::add(const QString& proxyParticipantId,
                             const QString& providerParticipantId,
                             QSharedPointer<RequestCaller> requestCaller,
                             SubscriptionRequest& subscriptionRequest,
                             IPublicationSender* publicationSender)
{
    assert(!requestCaller.isNull());
    QSharedPointer<SubscriptionRequestInformation> requestInfo(new SubscriptionRequestInformation(
            proxyParticipantId, providerParticipantId, subscriptionRequest));
    handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleAttributeSubscriptionRequest(
        QSharedPointer<SubscriptionRequestInformation> requestInfo,
        QSharedPointer<RequestCaller> requestCaller,
        IPublicationSender* publicationSender)
{
    QString subscriptionId = requestInfo->getSubscriptionId();
    QSharedPointer<Publication> publication(new Publication(publicationSender, requestCaller));

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscriptionList created
    // within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);

    if (publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  "Publication with id: " + requestInfo->getSubscriptionId() +
                          " already exists. Updating...");
        removeAttributePublication(subscriptionId);
    }

    subscriptionId2SubscriptionRequest.insert(subscriptionId, requestInfo);
    // Make note of the publication
    publications.insert(subscriptionId, publication);

    QList<QVariant> subscriptionList(subscriptionMapToListCopy(subscriptionId2SubscriptionRequest));

    // writing to subscriptions data structure done
    subscriptionLocker.unlock();

    LOG_DEBUG(logger, QString("added subscription: %1").arg(requestInfo->toQString()));

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        // Add an onChange publication if needed
        addOnChangePublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        QSharedPointer<SubscriptionQos> qos = requestInfo->getQos();
        qint64 publicationEndDelay = qos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch();

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
                publication->publicationEndRunnableHandle = delayedScheduler->schedule(
                        new PublicationEndRunnable(*this, subscriptionId), publicationEndDelay);
                LOG_DEBUG(
                        logger, QString("publication will end in %1 ms").arg(publicationEndDelay));
            }
            {
                QMutexLocker currentScheduledLocker(&currentScheduledPublicationsMutex);
                currentScheduledPublications.append(subscriptionId);
            }
            // sent at least once the current value
            delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId), -1);
        } else {
            LOG_WARN(logger, QString("publication end is in the past"));
        }
    }
    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::addOnChangePublication(
        const QString& subscriptionId,
        QSharedPointer<SubscriptionRequestInformation> request,
        QSharedPointer<Publication> publication)
{
    QMutexLocker publicationLocker(&(publication->mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos().data())) {
        LOG_TRACE(logger, QString("adding onChange subscription: %1").arg(subscriptionId));

        // Create an attribute listener to listen for onChange events
        SubscriptionAttributeListener* attributeListener =
                new SubscriptionAttributeListener(subscriptionId, *this);

        // Register the attribute listener
        QSharedPointer<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->registerAttributeListener(
                request->getSubscribeToName().toStdString(), attributeListener);

        // Make note of the attribute listener so that it can be unregistered
        publication->attributeListener = attributeListener;
    }
}

void PublicationManager::addBroadcastPublication(
        const QString& subscriptionId,
        QSharedPointer<BroadcastSubscriptionRequestInformation> request,
        QSharedPointer<PublicationManager::Publication> publication)
{
    LOG_TRACE(logger, QString("adding broadcast subscription: %1").arg(subscriptionId));

    QMutexLocker publicationLocker(&(publication->mutex));

    // Create a broadcast listener to listen for broadcast events
    SubscriptionBroadcastListener* broadcastListener =
            new SubscriptionBroadcastListener(subscriptionId, *this);

    // Register the broadcast listener
    QSharedPointer<RequestCaller> requestCaller = publication->requestCaller;
    requestCaller->registerBroadcastListener(
            request->getSubscribeToName().toStdString(), broadcastListener);

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
    QSharedPointer<SubscriptionRequestInformation> requestInfo(new SubscriptionRequestInformation(
            proxyParticipantId, providerParticipantId, subscriptionRequest));
    {
        QMutexLocker queueLocker(&queuedSubscriptionRequestsMutex);
        queuedSubscriptionRequests.insert(requestInfo->getProviderId(), requestInfo);
    }

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscriptionList created
    // within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);
    subscriptionId2SubscriptionRequest.insert(requestInfo->getSubscriptionId(), requestInfo);
    QList<QVariant> subscriptionList(subscriptionMapToListCopy(subscriptionId2SubscriptionRequest));
    subscriptionLocker.unlock();

    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::add(const QString& proxyParticipantId,
                             const QString& providerParticipantId,
                             QSharedPointer<RequestCaller> requestCaller,
                             BroadcastSubscriptionRequest& subscriptionRequest,
                             IPublicationSender* publicationSender)
{
    assert(!requestCaller.isNull());
    QSharedPointer<BroadcastSubscriptionRequestInformation> requestInfo(
            new BroadcastSubscriptionRequestInformation(
                    proxyParticipantId, providerParticipantId, subscriptionRequest));

    handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleBroadcastSubscriptionRequest(
        QSharedPointer<BroadcastSubscriptionRequestInformation> requestInfo,
        QSharedPointer<RequestCaller> requestCaller,
        IPublicationSender* publicationSender)
{

    QString subscriptionId = requestInfo->getSubscriptionId();

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscriptionList created
    // within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);
    QSharedPointer<Publication> publication(new Publication(publicationSender, requestCaller));

    if (publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  "Publication with id: " + requestInfo->getSubscriptionId() +
                          " already exists. Updating...");
        removeBroadcastPublication(subscriptionId);
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(subscriptionId, requestInfo);

    // Make note of the publication
    publications.insert(subscriptionId, publication);
    LOG_DEBUG(logger, QString("added subscription: %1").arg(requestInfo->toQString()));

    QList<QVariant> subscriptionList(
            subscriptionMapToListCopy(subscriptionId2BroadcastSubscriptionRequest));

    // writing to subscriptions data structure done
    subscriptionLocker.unlock();

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        // Add an onChange publication if needed
        addBroadcastPublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        QSharedPointer<SubscriptionQos> qos = requestInfo->getQos();
        qint64 publicationEndDelay = qos->getExpiryDate() - QDateTime::currentMSecsSinceEpoch();

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
    QSharedPointer<BroadcastSubscriptionRequestInformation> requestInfo(
            new BroadcastSubscriptionRequestInformation(
                    proxyParticipantId, providerParticipantId, subscriptionRequest));
    {
        QMutexLocker queueLocker(&queuedBroadcastSubscriptionRequestsMutex);
        queuedBroadcastSubscriptionRequests.insert(requestInfo->getProviderId(), requestInfo);
    }

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscriptionList created
    // within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);
    subscriptionId2BroadcastSubscriptionRequest.insert(
            requestInfo->getSubscriptionId(), requestInfo);
    QList<QVariant> subscriptionList(
            subscriptionMapToListCopy(subscriptionId2BroadcastSubscriptionRequest));
    subscriptionLocker.unlock();

    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeAllSubscriptions(const QString& providerId)
{
    LOG_DEBUG(logger, QString("Removing all subscriptions for provider id= %1").arg(providerId));

    // Build lists of subscriptionIds to remove
    QString subscriptionId;

    QList<QString> publicationsToRemove;
    {
        QReadLocker subscriptionLocker(&subscriptionLock);

        foreach (QSharedPointer<SubscriptionRequestInformation> requestInfo,
                 subscriptionId2SubscriptionRequest) {
            subscriptionId = requestInfo->getSubscriptionId();

            if (requestInfo->getProviderId() == providerId) {
                publicationsToRemove.append(subscriptionId);
            }
        }
    }

    QList<QString> broadcastsToRemove;
    {
        QReadLocker subscriptionLocker(&subscriptionLock);

        foreach (QSharedPointer<BroadcastSubscriptionRequestInformation> requestInfo,
                 subscriptionId2BroadcastSubscriptionRequest) {
            subscriptionId = requestInfo->getSubscriptionId();

            if (requestInfo->getProviderId() == providerId) {
                broadcastsToRemove.append(subscriptionId);
            }
        }
    }

    // Remove each publication
    foreach (subscriptionId, publicationsToRemove) {
        LOG_DEBUG(logger,
                  QString("Removing subscription providerId= %1, subscriptionId =%2")
                          .arg(providerId)
                          .arg(subscriptionId));
        removeAttributePublication(subscriptionId);
    }

    // Remove each broadcast
    foreach (subscriptionId, broadcastsToRemove) {
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

bool PublicationManager::publicationExists(const QString& subscriptionId) const
{
    return publications.contains(subscriptionId);
}

void PublicationManager::restore(const QString& providerId,
                                 QSharedPointer<RequestCaller> requestCaller,
                                 IPublicationSender* publicationSender)
{
    LOG_DEBUG(logger, "restore: entering ...");

    {
        QMutexLocker queueLocker(&queuedSubscriptionRequestsMutex);
        while (queuedSubscriptionRequests.contains(providerId)) {
            QSharedPointer<SubscriptionRequestInformation> requestInfo(
                    queuedSubscriptionRequests.take(providerId));
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                LOG_DEBUG(logger,
                          QString("Restoring subscription for provider: %1 %2").arg(providerId).arg(
                                  requestInfo->toQString()));
                handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
        }
    }

    {
        QMutexLocker queueLocker(&queuedBroadcastSubscriptionRequestsMutex);
        while (queuedBroadcastSubscriptionRequests.contains(providerId)) {
            QSharedPointer<BroadcastSubscriptionRequestInformation> requestInfo(
                    queuedBroadcastSubscriptionRequests.take(providerId));
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                LOG_DEBUG(logger,
                          QString("Restoring subscription for provider: %1 %2").arg(providerId).arg(
                                  requestInfo->toQString()));
                handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
        }
    }
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveAttributeSubscriptionRequestsMap(
        const QList<QVariant>& subscriptionList)
{
    LOG_DEBUG(logger, "Saving active attribute subscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionList, subscriptionRequestStorageFileName);
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
        const QList<QVariant>& subscriptionList)
{
    LOG_DEBUG(logger, "Saving active broadcastSubscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionList, broadcastSubscriptionRequestStorageFileName);
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
        const QMap<QString, QSharedPointer<RequestInformationType>>& map)
{
    QList<QVariant> subscriptionList;
    {
        foreach (QSharedPointer<RequestInformationType> requestInfo, map) {
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                subscriptionList.append(QVariant::fromValue(*requestInfo));
            }
        }
    }
    return subscriptionList;
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveSubscriptionRequestsMap(const QList<QVariant>& subscriptionList,
                                                     const QString& storageFilename)
{

    if (isShuttingDown()) {
        LOG_DEBUG(logger, "Abort saving, because we are already shutting down.");
        return;
    }

    {
        QMutexLocker fileWritelocker(&fileWriteLock);
        QFile file(storageFilename);
        if (!file.open(QIODevice::WriteOnly)) {
            LOG_ERROR(logger,
                      QString("Could not open subscription request storage file: %1")
                              .arg(file.errorString()));
            return;
        }

        // Write the subscription information as a json list
        file.resize(0);

        QString json = JsonSerializer::serialize(subscriptionList);
        file.write(json.toUtf8().constData());
    }
}

template <class RequestInformationType>
void PublicationManager::loadSavedSubscriptionRequestsMap(
        const QString& storageFilename,
        QMutex& queueMutex,
        QMultiMap<QString, QSharedPointer<RequestInformationType>>& queuedSubscriptions)
{

    static_assert(std::is_base_of<SubscriptionRequest, RequestInformationType>::value,
                  "loadSavedSubscriptionRequestsMap can only be used for subclasses of "
                  "SubscriptionRequest");

    QFile file(storageFilename);
    if (!file.open(QIODevice::ReadOnly)) {
        LOG_ERROR(logger, QString("Unable to read file: %1").arg(file.errorString()));
        return;
    }

    // Read the Json into memory
    QByteArray jsonBytes = file.readAll();

    // Deserialize the JSON into a list of subscription requests
    QList<RequestInformationType*> subscriptionList =
            JsonSerializer::deserializeList<RequestInformationType>(jsonBytes);

    // Loop through the saved subscriptions
    QDateTime now = QDateTime::currentDateTime();
    QMutexLocker queueLocker(&queueMutex);

    while (!subscriptionList.isEmpty()) {
        QSharedPointer<RequestInformationType> requestInfo(subscriptionList.takeFirst());

        // Add the subscription if it is still valid
        if (!isSubscriptionExpired(requestInfo->getQos())) {
            QString providerId = requestInfo->getProviderId();
            queuedSubscriptions.insertMulti(providerId, requestInfo);
            LOG_DEBUG(logger,
                      QString("Queuing subscription Request: %1 : %2").arg(providerId).arg(
                              requestInfo->toQString()));
        }
    }

    queueLocker.unlock();
}

void PublicationManager::removeAttributePublication(const QString& subscriptionId)
{
    LOG_DEBUG(logger, QString("removePublication: %1").arg(subscriptionId));

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscriptionList created
    // within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);

    if (!publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  QString("publication %1 does not exist - will not remove").arg(subscriptionId));
        return;
    }

    QSharedPointer<Publication> publication(publications.take(subscriptionId));
    QSharedPointer<SubscriptionRequestInformation> request(
            subscriptionId2SubscriptionRequest.take(subscriptionId));

    QList<QVariant> subscriptionList(subscriptionMapToListCopy(subscriptionId2SubscriptionRequest));
    subscriptionLocker.unlock();

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        // Delete the onChange publication if needed
        removeOnChangePublication(subscriptionId, request, publication);
    }

    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeBroadcastPublication(const QString& subscriptionId)
{
    LOG_DEBUG(logger, QString("removeBroadcast: %1").arg(subscriptionId));

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the subscriptionList created
    // within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);

    if (!publicationExists(subscriptionId)) {
        LOG_DEBUG(logger,
                  QString("publication %1 does not exist - will not remove").arg(subscriptionId));
        return;
    }

    QSharedPointer<Publication> publication(publications.take(subscriptionId));
    QSharedPointer<BroadcastSubscriptionRequestInformation> request(
            subscriptionId2BroadcastSubscriptionRequest.take(subscriptionId));

    QList<QVariant> subscriptionList(subscriptionMapToListCopy(subscriptionId2SubscriptionRequest));
    subscriptionLocker.unlock();

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        // Remove listener
        QSharedPointer<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterBroadcastListener(
                request->getSubscribeToName().toStdString(), publication->broadcastListener);
        publication->broadcastListener = NULL;

        removePublicationEndRunnable(publication);
    }

    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeOnChangePublication(
        const QString& subscriptionId,
        QSharedPointer<SubscriptionRequestInformation> request,
        QSharedPointer<Publication> publication)
{
    QMutexLocker publicationLocker(&(publication->mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos().data())) {
        LOG_DEBUG(logger, QString("Removing onChange publication for id = %1").arg(subscriptionId));

        // Unregister and delete the attribute listener
        QSharedPointer<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterAttributeListener(
                request->getSubscribeToName().toStdString(), publication->attributeListener);
        publication->attributeListener = NULL;
    }
    removePublicationEndRunnable(publication);
}

// This function assumes a write lock is alrady held for the publication}
void PublicationManager::removePublicationEndRunnable(QSharedPointer<Publication> publication)
{
    if (publication->publicationEndRunnableHandle != DelayedScheduler::INVALID_RUNNABLE_HANDLE() &&
        !isShuttingDown()) {
        LOG_DEBUG(logger,
                  QString("Unscheduling PublicationEndRunnable with handle: %1")
                          .arg(publication->publicationEndRunnableHandle));
        delayedScheduler->unschedule(publication->publicationEndRunnableHandle);
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE();
    }
}

// This function assumes that a read lock is already held
bool PublicationManager::processFilterChain(const QString& subscriptionId,
                                            const QList<QVariant>& broadcastValues,
                                            const QList<QSharedPointer<IBroadcastFilter>>& filters)
{
    bool success = true;

    QReadLocker subscriptionLocker(&subscriptionLock);
    QSharedPointer<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));
    BroadcastFilterParameters filterParameters = subscriptionRequest->getFilterParameters();

    foreach (QSharedPointer<IBroadcastFilter> filter, filters) {
        success = success &&
                  filter->filter(
                          broadcastValues, BroadcastFilterParameters::createStd(filterParameters));
    }

    return success;
}

bool PublicationManager::isShuttingDown()
{
    QMutexLocker shuwDownLocker(&shutDownMutex);
    return shuttingDown;
}

qint64 PublicationManager::getPublicationTtl(
        QSharedPointer<SubscriptionRequest> subscriptionRequest) const
{
    return subscriptionRequest->getQos()->getPublicationTtl();
}

void PublicationManager::sendPublication(
        QSharedPointer<Publication> publication,
        QSharedPointer<SubscriptionInformation> subscriptionInformation,
        QSharedPointer<SubscriptionRequest> request,
        const QVariant& value)
{
    LOG_DEBUG(logger, "sending subscriptionreply");
    MessagingQos mQos;

    QMutexLocker publicationLocker(&(publication->mutex));
    // Set the TTL
    mQos.setTtl(getPublicationTtl(request));

    IPublicationSender* publicationSender = publication->sender;

    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setResponse(value);
    publicationSender->sendSubscriptionPublication(
            subscriptionInformation->getProviderId().toStdString(),
            subscriptionInformation->getProxyId().toStdString(),
            mQos,
            subscriptionPublication);

    // Make note of when this publication was sent
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    publication->timeOfLastPublication = now;

    {
        QMutexLocker currentScheduledLocker(&currentScheduledPublicationsMutex);
        currentScheduledPublications.removeAll(request->getSubscriptionId());
    }
    LOG_TRACE(logger, QString("sent subscriptionreply @ %1").arg(now));
}

void PublicationManager::pollSubscription(const QString& subscriptionId)
{
    LOG_TRACE(logger, QString("pollSubscription %1").arg(subscriptionId));

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the publication and
    // subscriptionRequest object within the locked code is used after the unlock.
    QWriteLocker subscriptionLocker(&subscriptionLock);

    // Check that the subscription has not been removed and that we are not shutting down
    if (isShuttingDown() || !publicationExists(subscriptionId) ||
        !subscriptionId2SubscriptionRequest.contains(subscriptionId)) {
        return;
    }

    // Get the subscription details
    QSharedPointer<Publication> publication(publications.value(subscriptionId));
    QSharedPointer<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId));

    subscriptionLocker.unlock();

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        // See if the publication is needed
        QSharedPointer<SubscriptionQos> qos(subscriptionRequest->getQos());
        qint64 now = QDateTime::currentMSecsSinceEpoch();
        qint64 publicationInterval = SubscriptionUtil::getPeriodicPublicationInterval(qos.data());

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
        QString attributeGetter(
                Util::attributeGetterFromName(subscriptionRequest->getSubscribeToName()));
        QSharedPointer<RequestCaller> requestCaller(publication->requestCaller);
        QSharedPointer<IRequestInterpreter> requestInterpreter(
                InterfaceRegistrar::instance().getRequestInterpreter(
                        requestCaller->getInterfaceName()));

        std::function<void(const QList<QVariant>&)> callbackFct =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const QList<QVariant>& response) {
            sendPublication(publication, subscriptionRequest, subscriptionRequest, response.at(0));

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                LOG_DEBUG(logger,
                          QString("rescheduling runnable with delay: %1").arg(publicationInterval));
                delayedScheduler->schedule(
                        new PublisherRunnable(*this, subscriptionId), publicationInterval);
            }
        };

        LOG_DEBUG(logger, QString("run: executing requestInterpreter= %1").arg(attributeGetter));
        requestInterpreter->execute(
                requestCaller, attributeGetter, QList<QVariant>(), QList<QVariant>(), callbackFct);
    }
}

void PublicationManager::removePublication(const QString& subscriptionId)
{
    QWriteLocker subscriptionLocker(&subscriptionLock);
    if (subscriptionId2SubscriptionRequest.contains(subscriptionId)) {
        removeAttributePublication(subscriptionId);
    } else {
        removeBroadcastPublication(subscriptionId);
    }
}

void PublicationManager::attributeValueChanged(const QString& subscriptionId, const QVariant& value)
{
    LOG_DEBUG(logger,
              QString("attributeValueChanged for onChange subscription %1").arg(subscriptionId));

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the publication and
    // subscriptionRequest object is used after the unlock.
    QReadLocker subscriptionLocker(&subscriptionLock);

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        LOG_ERROR(logger,
                  QString("attributeValueChanged called for non-existing subscription %1")
                          .arg(subscriptionId));
        return;
    }

    QSharedPointer<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId));

    QSharedPointer<Publication> publication(publications.value(subscriptionId));
    subscriptionLocker.unlock();

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        if (!isPublicationAlreadyScheduled(subscriptionId)) {
            qint64 timeUntilNextPublication =
                    getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

            if (timeUntilNextPublication == 0) {
                // Send the publication
                sendPublication(publication, subscriptionRequest, subscriptionRequest, value);
            } else {
                reschedulePublication(subscriptionId, timeUntilNextPublication);
            }
        }
    }
}

void PublicationManager::broadcastOccurred(const QString& subscriptionId,
                                           const QList<QVariant>& values,
                                           const QList<QSharedPointer<IBroadcastFilter>>& filters)
{
    LOG_DEBUG(logger,
              QString("broadcastOccurred for subscription %1. Number of values: %2")
                      .arg(subscriptionId)
                      .arg(values.size()));

    // lock the access to the subscriptions data structure
    // we don't use a separate block for locking/unlocking, because the publication and
    // subscriptionRequest object is used after the unlock.
    QReadLocker subscriptionLocker(&subscriptionLock);

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        LOG_ERROR(logger,
                  QString("broadcastOccurred called for non-existing subscription %1")
                          .arg(subscriptionId));
        return;
    }

    QSharedPointer<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));
    QSharedPointer<Publication> publication(publications.value(subscriptionId));
    subscriptionLocker.unlock();

    {
        QMutexLocker publicationLocker(&(publication->mutex));
        // Only proceed if publication can immediately be sent
        qint64 timeUntilNextPublication =
                getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

        if (timeUntilNextPublication == 0) {
            // Execute broadcast filters
            if (processFilterChain(subscriptionId, values, filters)) {
                // Send the publication
                QVariant value = QVariant::fromValue(values);
                sendPublication(publication, subscriptionRequest, subscriptionRequest, value);
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
    QMutexLocker currentScheduledLocker(&currentScheduledPublicationsMutex);
    return currentScheduledPublications.contains(subscriptionId);
}

qint64 PublicationManager::getTimeUntilNextPublication(QSharedPointer<Publication> publication,
                                                       QSharedPointer<SubscriptionQos> qos)
{
    QMutexLocker publicationLocker(&(publication->mutex));
    // Check the last publication time against the min interval
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    qint64 minInterval = SubscriptionUtil::getMinInterval(qos.data());

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
        QMutexLocker currentScheduledLocker(&currentScheduledPublicationsMutex);

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
                                             QSharedPointer<RequestCaller> requestCaller)
        : timeOfLastPublication(0),
          sender(publicationSender),
          requestCaller(requestCaller),
          attributeListener(NULL),
          broadcastListener(NULL),
          mutex(QMutex::RecursionMode::Recursive),
          publicationEndRunnableHandle(DelayedScheduler::INVALID_RUNNABLE_HANDLE())
{
}

//------ PublicationManager::PublisherRunnable ---------------------------------

PublicationManager::PublisherRunnable::~PublisherRunnable()
{
}

PublicationManager::PublisherRunnable::PublisherRunnable(PublicationManager& publicationManager,
                                                         const QString& subscriptionId)
        : publicationManager(publicationManager), subscriptionId(subscriptionId)
{
    setAutoDelete(true);
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
        : publicationManager(publicationManager), subscriptionId(subscriptionId)
{
    setAutoDelete(true);
}

void PublicationManager::PublicationEndRunnable::run()
{
    QWriteLocker subscriptionsLocker(&publicationManager.subscriptionLock);
    if (!publicationManager.publicationExists(subscriptionId)) {
        return;
    }
    QSharedPointer<Publication> publication(publicationManager.publications.value(subscriptionId));
    publicationManager.removePublication(subscriptionId);
    subscriptionsLocker.unlock();

    {
        QMutexLocker locker(&(publication->mutex));
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE();
    }
}

} // namespace joynr
