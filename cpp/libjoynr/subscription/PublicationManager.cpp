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

#include <cassert>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <chrono>
#include <mutex>

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
#include "joynr/Util.h"
#include "joynr/LibjoynrSettings.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/Request.h"
#include "joynr/Reply.h"

namespace joynr
{

INIT_LOGGER(PublicationManager);

class PublicationManager::PublisherRunnable : public Runnable
{
public:
    ~PublisherRunnable() override = default;
    PublisherRunnable(PublicationManager& publicationManager, const std::string& subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::pollSubscription()
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(PublisherRunnable);
    PublicationManager& publicationManager;
    std::string subscriptionId;
};

class PublicationManager::PublicationEndRunnable : public Runnable
{
public:
    ~PublicationEndRunnable() override = default;
    PublicationEndRunnable(PublicationManager& publicationManager,
                           const std::string& subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::removePublication()
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(PublicationEndRunnable);
    PublicationManager& publicationManager;
    std::string subscriptionId;
};

//------ PublicationManager ----------------------------------------------------

PublicationManager::~PublicationManager()
{
    JOYNR_LOG_DEBUG(logger, "Destructor, saving subscriptionsMap...");

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

    JOYNR_LOG_DEBUG(logger, "Destructor, shutting down for thread pool and scheduler ...");
    delayedScheduler->shutdown();

    // Remove all publications
    JOYNR_LOG_DEBUG(logger, "Destructor: removing publications");

    while (subscriptionId2SubscriptionRequest.size() > 0) {
        auto subscriptionRequest = subscriptionId2SubscriptionRequest.begin();
        removeAttributePublication((subscriptionRequest->second)->getSubscriptionId(), false);
    }

    while (subscriptionId2BroadcastSubscriptionRequest.size() > 0) {
        auto broadcastRequest = subscriptionId2BroadcastSubscriptionRequest.begin();
        removeBroadcastPublication((broadcastRequest->second)->getSubscriptionId(), false);
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
          subscriptionRequestStorageFileName(),
          broadcastSubscriptionRequestStorageFileName(),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilters(),
          broadcastFilterLock()
{
}

PublicationManager::PublicationManager(int maxThreads)
        : publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          fileWriteLock(),
          delayedScheduler(new ThreadPoolDelayedScheduler(maxThreads, "PubManager")),
          shutDownMutex(),
          shuttingDown(false),
          subscriptionRequestStorageFileName(),
          broadcastSubscriptionRequestStorageFileName(),
          queuedSubscriptionRequests(),
          queuedSubscriptionRequestsMutex(),
          queuedBroadcastSubscriptionRequests(),
          queuedBroadcastSubscriptionRequestsMutex(),
          currentScheduledPublications(),
          currentScheduledPublicationsMutex(),
          broadcastFilters(),
          broadcastFilterLock()
{
}

bool isSubscriptionExpired(const SubscriptionQos* qos, int offset = 0)
{
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    return qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE() &&
           qos->getExpiryDateMs() < (now + offset);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             SubscriptionRequest& subscriptionRequest,
                             IPublicationSender* publicationSender)
{
    assert(requestCaller);
    auto requestInfo = std::make_shared<SubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);
    handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleAttributeSubscriptionRequest(
        std::shared_ptr<SubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        IPublicationSender* publicationSender)
{
    std::string subscriptionId = requestInfo->getSubscriptionId();
    auto publication = std::make_shared<Publication>(publicationSender, requestCaller);

    if (publicationExists(subscriptionId)) {
        JOYNR_LOG_DEBUG(logger,
                        "Publication with id: {}  already exists. Updating...",
                        requestInfo->getSubscriptionId());
        removeAttributePublication(subscriptionId);
    }

    subscriptionId2SubscriptionRequest.insert(subscriptionId, requestInfo);
    // Make note of the publication
    publications.insert(subscriptionId, publication);

    std::vector<Variant> subscriptionVector(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    JOYNR_LOG_DEBUG(logger, "added subscription: {}", requestInfo->toString());

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Add an onChange publication if needed
        addOnChangePublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const SubscriptionQos* qos = requestInfo->getSubscriptionQosPtr();
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        std::int64_t publicationEndDelay = qos->getExpiryDateMs() - now;

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE()) {
                publication->publicationEndRunnableHandle = delayedScheduler->schedule(
                        new PublicationEndRunnable(*this, subscriptionId),
                        std::chrono::milliseconds(publicationEndDelay));
                JOYNR_LOG_DEBUG(logger, "publication will end in {}  ms", publicationEndDelay);
            }
            {
                std::lock_guard<std::mutex> currentScheduledLocker(
                        currentScheduledPublicationsMutex);
                currentScheduledPublications.push_back(subscriptionId);
            }
            // sent at least once the current value
            delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId));
        } else {
            JOYNR_LOG_WARN(logger, "publication end is in the past");
        }
    }
    saveAttributeSubscriptionRequestsMap(subscriptionVector);
}

void PublicationManager::addOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        JOYNR_LOG_TRACE(logger, "adding onChange subscription: {}", subscriptionId);

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
        const std::string& subscriptionId,
        std::shared_ptr<BroadcastSubscriptionRequestInformation> request,
        std::shared_ptr<PublicationManager::Publication> publication)
{
    JOYNR_LOG_TRACE(logger, "adding broadcast subscription: {}", subscriptionId);

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

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             SubscriptionRequest& subscriptionRequest)
{
    JOYNR_LOG_DEBUG(logger,
                    "Added subscription for non existing provider (adding subscriptionRequest "
                    "to queue).");
    auto requestInfo = std::make_shared<SubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);
    {
        std::lock_guard<std::mutex> queueLocker(queuedSubscriptionRequestsMutex);
        queuedSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId(), requestInfo));
    }

    subscriptionId2SubscriptionRequest.insert(requestInfo->getSubscriptionId(), requestInfo);
    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             BroadcastSubscriptionRequest& subscriptionRequest,
                             IPublicationSender* publicationSender)
{
    assert(requestCaller);
    auto requestInfo = std::make_shared<BroadcastSubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);

    handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleBroadcastSubscriptionRequest(
        std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        IPublicationSender* publicationSender)
{

    std::string subscriptionId = requestInfo->getSubscriptionId();

    auto publication = std::make_shared<Publication>(publicationSender, requestCaller);

    if (publicationExists(subscriptionId)) {
        JOYNR_LOG_DEBUG(logger,
                        "Publication with id: {}  already exists. Updating...",
                        requestInfo->getSubscriptionId());
        removeBroadcastPublication(subscriptionId);
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(subscriptionId, requestInfo);

    // Make note of the publication
    publications.insert(subscriptionId, publication);
    JOYNR_LOG_DEBUG(logger, "added subscription: {}", requestInfo->toString());

    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Add an onChange publication if needed
        addBroadcastPublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const SubscriptionQos* qos = requestInfo->getSubscriptionQosPtr();
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        std::int64_t publicationEndDelay = qos->getExpiryDateMs() - now;

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE()) {
                publication->publicationEndRunnableHandle = delayedScheduler->schedule(
                        new PublicationEndRunnable(*this, subscriptionId),
                        std::chrono::milliseconds(publicationEndDelay));
                JOYNR_LOG_DEBUG(logger, "publication will end in {}  ms", publicationEndDelay);
            }
        } else {
            JOYNR_LOG_WARN(logger, "publication end is in the past");
        }
    }
    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             BroadcastSubscriptionRequest& subscriptionRequest)
{
    JOYNR_LOG_DEBUG(logger,
                    "Added broadcast subscription for non existing provider (adding "
                    "subscriptionRequest to queue).");
    auto requestInfo = std::make_shared<BroadcastSubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);
    {
        std::lock_guard<std::mutex> queueLocker(queuedBroadcastSubscriptionRequestsMutex);
        queuedBroadcastSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId(), requestInfo));
    }

    subscriptionId2BroadcastSubscriptionRequest.insert(
            requestInfo->getSubscriptionId(), requestInfo);
    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));

    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeAllSubscriptions(const std::string& providerId)
{
    JOYNR_LOG_DEBUG(logger, "Removing all subscriptions for provider id= {}", providerId);

    // Build lists of subscriptionIds to remove
    std::string subscriptionId;

    std::vector<std::string> publicationsToRemove;
    {
        for (auto& requestInfo : subscriptionId2SubscriptionRequest) {
            subscriptionId = requestInfo.second->getSubscriptionId();

            if ((requestInfo.second)->getProviderId() == providerId) {
                publicationsToRemove.push_back(subscriptionId);
            }
        }
    }

    std::vector<std::string> broadcastsToRemove;
    {
        for (auto& requestInfo : subscriptionId2BroadcastSubscriptionRequest) {
            subscriptionId = requestInfo.second->getSubscriptionId();

            if ((requestInfo.second)->getProviderId() == providerId) {
                broadcastsToRemove.push_back(subscriptionId);
            }
        }
    }

    // Remove each publication
    for (const std::string& subscriptionId : publicationsToRemove) {
        JOYNR_LOG_DEBUG(logger,
                        "Removing subscription providerId= {}, subscriptionId = {}",
                        providerId,
                        subscriptionId);
        removeAttributePublication(subscriptionId);
    }

    // Remove each broadcast
    for (const std::string& subscriptionId : broadcastsToRemove) {
        JOYNR_LOG_DEBUG(logger,
                        "Removing subscription providerId= {}, subscriptionId = {}",
                        providerId,
                        subscriptionId);
        removeBroadcastPublication(subscriptionId);
    }
}

void PublicationManager::stopPublication(const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger, "stopPublication: {}", subscriptionId);
    removePublication(subscriptionId);
}

bool PublicationManager::publicationExists(const std::string& subscriptionId) const
{
    return publications.contains(subscriptionId);
}

void PublicationManager::restore(const std::string& providerId,
                                 std::shared_ptr<RequestCaller> requestCaller,
                                 IPublicationSender* publicationSender)
{
    JOYNR_LOG_DEBUG(logger, "restore: entering ...");

    {
        std::lock_guard<std::mutex> queueLocker(queuedSubscriptionRequestsMutex);
        std::multimap<std::string, std::shared_ptr<SubscriptionRequestInformation>>::iterator
                queuedSubscriptionRequestsIterator = queuedSubscriptionRequests.find(providerId);
        while (queuedSubscriptionRequestsIterator != queuedSubscriptionRequests.end()) {
            std::shared_ptr<SubscriptionRequestInformation> requestInfo(
                    queuedSubscriptionRequestsIterator->second);
            queuedSubscriptionRequests.erase(queuedSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
                JOYNR_LOG_DEBUG(logger,
                                "Restoring subscription for provider: {} {}",
                                providerId,
                                requestInfo->toString());
                handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedSubscriptionRequestsIterator = queuedSubscriptionRequests.find(providerId);
        }
    }

    {
        std::lock_guard<std::mutex> queueLocker(queuedBroadcastSubscriptionRequestsMutex);
        std::multimap<std::string,
                      std::shared_ptr<BroadcastSubscriptionRequestInformation>>::iterator
                queuedBroadcastSubscriptionRequestsIterator =
                        queuedBroadcastSubscriptionRequests.find(providerId);
        while (queuedBroadcastSubscriptionRequestsIterator !=
               queuedBroadcastSubscriptionRequests.end()) {
            std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
                    queuedBroadcastSubscriptionRequestsIterator->second);
            queuedBroadcastSubscriptionRequests.erase(queuedBroadcastSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
                JOYNR_LOG_DEBUG(logger,
                                "Restoring subscription for provider: {}  {}",
                                providerId,
                                requestInfo->toString());
                handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedBroadcastSubscriptionRequestsIterator =
                    queuedBroadcastSubscriptionRequests.find(providerId);
        }
    }
}

void PublicationManager::loadSavedAttributeSubscriptionRequestsMap(const std::string& fileName)
{
    JOYNR_LOG_DEBUG(logger, "Loading stored AttributeSubscriptionrequests.");

    // update reference file
    if (fileName != subscriptionRequestStorageFileName) {
        subscriptionRequestStorageFileName = std::move(fileName);
    }

    loadSavedSubscriptionRequestsMap<SubscriptionRequestInformation>(
            subscriptionRequestStorageFileName,
            queuedSubscriptionRequestsMutex,
            queuedSubscriptionRequests);
}

void PublicationManager::loadSavedBroadcastSubscriptionRequestsMap(const std::string& fileName)
{
    JOYNR_LOG_DEBUG(logger, "Loading stored BroadcastSubscriptionrequests.");

    // update reference file
    if (fileName != broadcastSubscriptionRequestStorageFileName) {
        broadcastSubscriptionRequestStorageFileName = std::move(fileName);
    }

    loadSavedSubscriptionRequestsMap<BroadcastSubscriptionRequestInformation>(
            broadcastSubscriptionRequestStorageFileName,
            queuedBroadcastSubscriptionRequestsMutex,
            queuedBroadcastSubscriptionRequests);
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveBroadcastSubscriptionRequestsMap(
        const std::vector<Variant>& subscriptionVector)
{
    JOYNR_LOG_DEBUG(logger, "Saving active broadcastSubscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionVector, broadcastSubscriptionRequestStorageFileName);
}

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveAttributeSubscriptionRequestsMap(
        const std::vector<Variant>& subscriptionVector)
{
    JOYNR_LOG_DEBUG(logger, "Saving active attribute subscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionVector, subscriptionRequestStorageFileName);
}

template <class RequestInformationType>
std::vector<Variant> PublicationManager::subscriptionMapToVectorCopy(
        const ThreadSafeMap<std::string, std::shared_ptr<RequestInformationType>>& map)
{
    std::vector<Variant> subscriptionVector;
    {
        for (typename ThreadSafeMap<std::string,
                                    std::shared_ptr<RequestInformationType>>::MapIterator iterator =
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
                                                     const std::string& storageFilename)
{
    if (isShuttingDown()) {
        JOYNR_LOG_DEBUG(logger, "Abort saving, because we are already shutting down.");
        return;
    }

    try {
        joynr::util::saveStringToFile(
                storageFilename, JsonSerializer::serializeVector(subscriptionVector));
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }
}

template <class RequestInformationType>
void PublicationManager::loadSavedSubscriptionRequestsMap(
        const std::string& storageFilename,
        std::mutex& queueMutex,
        std::multimap<std::string, std::shared_ptr<RequestInformationType>>& queuedSubscriptions)
{

    static_assert(std::is_base_of<SubscriptionRequest, RequestInformationType>::value,
                  "loadSavedSubscriptionRequestsMap can only be used for subclasses of "
                  "SubscriptionRequest");

    std::string jsonString;
    try {
        jsonString = joynr::util::loadStringFromFile(storageFilename);
    } catch (const std::runtime_error& ex) {
        JOYNR_LOG_ERROR(logger, ex.what());
    }

    if (jsonString.empty()) {
        return;
    }

    // Deserialize the JSON into a list of subscription requests
    std::vector<RequestInformationType*> subscriptionVector =
            JsonSerializer::deserializeVector<RequestInformationType>(jsonString);

    // Loop through the saved subscriptions
    std::lock_guard<std::mutex> queueLocker(queueMutex);

    while (!subscriptionVector.empty()) {
        std::shared_ptr<RequestInformationType> requestInfo(*(subscriptionVector.begin()));
        subscriptionVector.erase(subscriptionVector.begin());

        // Add the subscription if it is still valid
        if (!isSubscriptionExpired(requestInfo->getSubscriptionQosPtr())) {
            std::string providerId = requestInfo->getProviderId();
            queuedSubscriptions.insert(std::make_pair(providerId, requestInfo));
            JOYNR_LOG_DEBUG(logger,
                            "Queuing subscription Request: {}  : {}",
                            providerId,
                            requestInfo->toString());
        }
    }
}

void PublicationManager::removeAttributePublication(const std::string& subscriptionId,
                                                    const bool updatePersistenceFile)
{
    JOYNR_LOG_DEBUG(logger, "removePublication: {}", subscriptionId);

    std::shared_ptr<Publication> publication(publications.take(subscriptionId));
    std::shared_ptr<SubscriptionRequestInformation> request(
            subscriptionId2SubscriptionRequest.take(subscriptionId));

    if (publication != nullptr && request != nullptr) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Delete the onChange publication if needed
        removeOnChangePublication(subscriptionId, request, publication);
    }

    if (updatePersistenceFile) {
        saveAttributeSubscriptionRequestsMap(
                subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));
    }
}

void PublicationManager::removeBroadcastPublication(const std::string& subscriptionId,
                                                    const bool updatePersistenceFile)
{
    JOYNR_LOG_DEBUG(logger, "removeBroadcast: {}", subscriptionId);

    std::shared_ptr<Publication> publication(publications.take(subscriptionId));

    std::shared_ptr<BroadcastSubscriptionRequestInformation> request(
            subscriptionId2BroadcastSubscriptionRequest.take(subscriptionId));

    if (publication != nullptr && request != nullptr) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Remove listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterBroadcastListener(
                request->getSubscribeToName(), publication->broadcastListener);
        publication->broadcastListener = nullptr;

        removePublicationEndRunnable(publication);
    }

    if (updatePersistenceFile) {
        saveBroadcastSubscriptionRequestsMap(
                subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));
    }
}

void PublicationManager::removeOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    JOYNR_LOG_DEBUG(logger, "Removing onChange publication for id = {}", subscriptionId);
    // to silence unused-variable compiler warnings
    std::ignore = subscriptionId;

    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        // Unregister and delete the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterAttributeListener(
                request->getSubscribeToName(), publication->attributeListener);
        publication->attributeListener = nullptr;
    }
    removePublicationEndRunnable(publication);
}

// This function assumes a write lock is alrady held for the publication}
void PublicationManager::removePublicationEndRunnable(std::shared_ptr<Publication> publication)
{
    if (publication->publicationEndRunnableHandle != DelayedScheduler::INVALID_RUNNABLE_HANDLE &&
        !isShuttingDown()) {
        JOYNR_LOG_DEBUG(logger,
                        "Unscheduling PublicationEndRunnable with handle: {}",
                        publication->publicationEndRunnableHandle);
        delayedScheduler->unschedule(publication->publicationEndRunnableHandle);
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

bool PublicationManager::isShuttingDown()
{
    std::lock_guard<std::mutex> shuwDownLocker(shutDownMutex);
    return shuttingDown;
}

std::int64_t PublicationManager::getPublicationTtlMs(
        std::shared_ptr<SubscriptionRequest> subscriptionRequest) const
{
    const SubscriptionQos* qosPtr = subscriptionRequest->getSubscriptionQosPtr();
    return qosPtr->getPublicationTtlMs();
}

void PublicationManager::sendPublicationError(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        std::shared_ptr<exceptions::JoynrRuntimeException> exception)
{
    JOYNR_LOG_DEBUG(logger, "sending subscription error");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setError(std::move(exception));
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, subscriptionPublication);
    JOYNR_LOG_DEBUG(logger, "sent subscription error");
}

void PublicationManager::sendSubscriptionPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        const SubscriptionPublication& subscriptionPublication)
{

    MessagingQos mQos;

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    // Set the TTL
    mQos.setTtl(getPublicationTtlMs(request));

    IPublicationSender* publicationSender = publication->sender;

    publicationSender->sendSubscriptionPublication(subscriptionInformation->getProviderId(),
                                                   subscriptionInformation->getProxyId(),
                                                   mQos,
                                                   subscriptionPublication);

    // Make note of when this publication was sent
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    publication->timeOfLastPublication = now;

    {
        std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
        util::removeAll(currentScheduledPublications, request->getSubscriptionId());
    }
    JOYNR_LOG_TRACE(logger, "sent publication @ {}", now);
}

void PublicationManager::sendPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        BaseReply&& response)
{
    JOYNR_LOG_DEBUG(logger, "sending subscription reply");
    SubscriptionPublication subscriptionPublication(std::move(response));
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, subscriptionPublication);
    JOYNR_LOG_TRACE(logger, "sent subscription reply");
}

void PublicationManager::pollSubscription(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger, "pollSubscription {}", subscriptionId);

    if (isShuttingDown() || !publicationExists(subscriptionId) ||
        !subscriptionId2SubscriptionRequest.contains(subscriptionId)) {
        return;
    }

    // Get the subscription details
    std::shared_ptr<Publication> publication(publications.value(subscriptionId));
    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId));
    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // See if the publication is needed
        const SubscriptionQos* qos = subscriptionRequest->getSubscriptionQosPtr();
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch()).count();
        std::int64_t publicationInterval =
                SubscriptionUtil::getPeriodicPublicationInterval(subscriptionRequest->getQos());

        // check if the subscription qos needs a periodic publication
        if (publicationInterval > 0) {
            std::int64_t timeSinceLast = now - publication->timeOfLastPublication;
            // publish only if not published in the current interval
            if (timeSinceLast < publicationInterval) {
                JOYNR_LOG_DEBUG(
                        logger,
                        "no publication necessary. publicationInterval: {}, timeSinceLast {}",
                        publicationInterval,
                        timeSinceLast);

                std::int64_t delayUntilNextPublication = publicationInterval - timeSinceLast;
                assert(delayUntilNextPublication >= 0);
                delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                           std::chrono::milliseconds(delayUntilNextPublication));
                return;
            }
        }

        // Get the value of the attribute
        std::string attributeGetter(
                util::attributeGetterFromName(subscriptionRequest->getSubscribeToName()));
        std::shared_ptr<RequestCaller> requestCaller(publication->requestCaller);
        std::shared_ptr<IRequestInterpreter> requestInterpreter(
                InterfaceRegistrar::instance().getRequestInterpreter(
                        requestCaller->getInterfaceName()));

        std::function<void(Reply && )> onSuccess =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        Reply&& response) {
            sendPublication(
                    publication, subscriptionRequest, subscriptionRequest, std::move(response));

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                JOYNR_LOG_DEBUG(
                        logger, "rescheduling runnable with delay: {}", publicationInterval);
                delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                           std::chrono::milliseconds(publicationInterval));
            }
        };

        std::function<void(const std::shared_ptr<exceptions::JoynrException>&)> onError =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const std::shared_ptr<exceptions::JoynrException>& exception) {

            std::shared_ptr<exceptions::JoynrRuntimeException> runtimeError =
                    std::dynamic_pointer_cast<exceptions::JoynrRuntimeException>(exception);
            assert(runtimeError);
            sendPublicationError(
                    publication, subscriptionRequest, subscriptionRequest, runtimeError);

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                JOYNR_LOG_DEBUG(
                        logger, "rescheduling runnable with delay: {}", publicationInterval);
                delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                           std::chrono::milliseconds(publicationInterval));
            }
        };

        JOYNR_LOG_DEBUG(logger, "run: executing requestInterpreter= {}", attributeGetter);
        Request dummyRequest;
        dummyRequest.setMethodName(attributeGetter);
        requestInterpreter->execute(requestCaller, dummyRequest, onSuccess, onError);
    }
}

void PublicationManager::removePublication(const std::string& subscriptionId)
{
    if (subscriptionId2SubscriptionRequest.contains(subscriptionId)) {
        removeAttributePublication(subscriptionId);
    } else if (subscriptionId2BroadcastSubscriptionRequest.contains(subscriptionId)) {
        removeBroadcastPublication(subscriptionId);
    }
}

bool PublicationManager::isPublicationAlreadyScheduled(const std::string& subscriptionId)
{
    std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
    return util::vectorContains(currentScheduledPublications, subscriptionId);
}

std::int64_t PublicationManager::getTimeUntilNextPublication(
        std::shared_ptr<Publication> publication,
        Variant qos)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    // Check the last publication time against the min interval
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    std::int64_t minInterval = SubscriptionUtil::getMinInterval(qos);

    std::int64_t timeSinceLast = now - publication->timeOfLastPublication;

    if (minInterval > 0 && timeSinceLast < minInterval) {
        return minInterval - timeSinceLast;
    }

    return 0;
}

void PublicationManager::reschedulePublication(const std::string& subscriptionId,
                                               std::int64_t nextPublication)
{
    if (nextPublication > 0) {
        std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);

        // Schedule a publication so that the change is not forgotten
        if (!util::vectorContains(currentScheduledPublications, subscriptionId)) {
            JOYNR_LOG_DEBUG(logger, "rescheduling runnable with delay: {}", nextPublication);
            currentScheduledPublications.push_back(subscriptionId);
            delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                       std::chrono::milliseconds(nextPublication));
        }
    }
}

//------ PublicationManager::Publication ---------------------------------------

PublicationManager::Publication::Publication(IPublicationSender* publicationSender,
                                             std::shared_ptr<RequestCaller> requestCaller)
        : timeOfLastPublication(0),
          sender(publicationSender),
          requestCaller(requestCaller),
          attributeListener(nullptr),
          broadcastListener(nullptr),
          mutex(),
          publicationEndRunnableHandle(DelayedScheduler::INVALID_RUNNABLE_HANDLE)
{
}

//------ PublicationManager::PublisherRunnable ---------------------------------

PublicationManager::PublisherRunnable::PublisherRunnable(PublicationManager& publicationManager,
                                                         const std::string& subscriptionId)
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

PublicationManager::PublicationEndRunnable::PublicationEndRunnable(
        PublicationManager& publicationManager,
        const std::string& subscriptionId)
        : Runnable(true), publicationManager(publicationManager), subscriptionId(subscriptionId)
{
}

void PublicationManager::PublicationEndRunnable::shutdown()
{
}

void PublicationManager::PublicationEndRunnable::run()
{
    std::shared_ptr<Publication> publication(publicationManager.publications.value(subscriptionId));
    publicationManager.removePublication(subscriptionId);

    if (publication != nullptr) {
        std::lock_guard<std::recursive_mutex> lock((publication->mutex));
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

} // namespace joynr
