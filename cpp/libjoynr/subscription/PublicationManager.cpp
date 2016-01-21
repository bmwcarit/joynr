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
#include "joynr/BroadcastFilterParameters.h"
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

#include <fstream>
#include <sstream>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <mutex>

namespace joynr
{

INIT_LOGGER(PublicationManager);

//------ HelperClasses ---------------------------------------------------------

class PublicationManager::Publication
{
public:
    Publication(IPublicationSender* publicationSender,
                std::shared_ptr<RequestCaller> requestCaller);
    ~Publication();

    std::int64_t timeOfLastPublication;
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
        removeAttributePublication((subscriptionRequest->second)->getSubscriptionId());
    }

    while (subscriptionId2BroadcastSubscriptionRequest.size() > 0) {
        auto broadcastRequest = subscriptionId2BroadcastSubscriptionRequest.begin();
        removeBroadcastPublication((broadcastRequest->second)->getSubscriptionId());
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
    loadSavedAttributeSubscriptionRequestsMap();
    loadSavedBroadcastSubscriptionRequestsMap();
}

PublicationManager::PublicationManager(int maxThreads)
        : publications(),
          subscriptionId2SubscriptionRequest(),
          subscriptionId2BroadcastSubscriptionRequest(),
          fileWriteLock(),
          delayedScheduler(new ThreadPoolDelayedScheduler(maxThreads, "PubManager")),
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
    loadSavedAttributeSubscriptionRequestsMap();
    loadSavedBroadcastSubscriptionRequestsMap();
}

bool isSubscriptionExpired(const SubscriptionQos* qos, int offset = 0)
{
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch()).count();
    return qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE() &&
           qos->getExpiryDate() < (now + offset);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
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
    std::string subscriptionId = requestInfo->getSubscriptionId();
    std::shared_ptr<Publication> publication(new Publication(publicationSender, requestCaller));

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
        std::int64_t publicationEndDelay = qos->getExpiryDate() - now;

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
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
    std::shared_ptr<SubscriptionRequestInformation> requestInfo(new SubscriptionRequestInformation(
            proxyParticipantId, providerParticipantId, subscriptionRequest));
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

    std::string subscriptionId = requestInfo->getSubscriptionId();

    std::shared_ptr<Publication> publication(new Publication(publicationSender, requestCaller));

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
        std::int64_t publicationEndDelay = qos->getExpiryDate() - now;

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            if (qos->getExpiryDate() != joynr::SubscriptionQos::NO_EXPIRY_DATE()) {
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
    std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
            new BroadcastSubscriptionRequestInformation(
                    proxyParticipantId, providerParticipantId, subscriptionRequest));
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

// This function assumes that subscriptionList is a copy that is exclusively used by this function
void PublicationManager::saveAttributeSubscriptionRequestsMap(
        const std::vector<Variant>& subscriptionVector)
{
    JOYNR_LOG_DEBUG(logger, "Saving active attribute subscriptionRequests to file.");

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
    JOYNR_LOG_DEBUG(logger, "Saving active broadcastSubscriptionRequests to file.");

    saveSubscriptionRequestsMap(subscriptionVector, broadcastSubscriptionRequestStorageFileName);
}

void PublicationManager::loadSavedBroadcastSubscriptionRequestsMap()
{
    JOYNR_LOG_DEBUG(logger, "Loading stored BroadcastSubscriptionrequests.");

    loadSavedSubscriptionRequestsMap<BroadcastSubscriptionRequestInformation>(
            broadcastSubscriptionRequestStorageFileName,
            queuedBroadcastSubscriptionRequestsMutex,
            queuedBroadcastSubscriptionRequests);
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
                                                     const std::string& storageFilename)
{
    if (isShuttingDown()) {
        JOYNR_LOG_DEBUG(logger, "Abort saving, because we are already shutting down.");
        return;
    }

    {
        std::lock_guard<std::mutex> fileWritelocker(fileWriteLock);
        std::fstream file;
        file.open(storageFilename, std::ios::out);
        if (!file.is_open()) {
            std::string error;
            if (file.rdstate() == std::ios_base::badbit) {
                error = "irrecoverable stream error";
            } else if (file.rdstate() == std::ios_base::failbit) {
                error = "input/output operation failed";
            } else if (file.rdstate() == std::ios_base::eofbit) {
                error = "associated input sequence has reached end-of-file";
            }
            JOYNR_LOG_ERROR(logger, "Could not open subscription request storage file: {}", error);
            return;
        }

        std::string json = JsonSerializer::serializeVector(subscriptionVector);
        file << json;
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

    std::fstream file;
    file.open(storageFilename, std::ios::in);
    if (!file.is_open()) {
        std::string error;
        if (file.rdstate() == std::ios_base::badbit) {
            error = "irrecoverable stream error";
        } else if (file.rdstate() == std::ios_base::failbit) {
            error = "input/output operation failed";
        } else if (file.rdstate() == std::ios_base::eofbit) {
            error = "associated input sequence has reached end-of-file";
        }
        JOYNR_LOG_ERROR(logger, "Unable to read file: {}, reson: {}", storageFilename, error);
        return;
    }

    // Read the Json into memory
    std::stringstream jsonBytes;
    jsonBytes << file.rdbuf();
    JOYNR_LOG_DEBUG(logger, "jsonBytes: {}", jsonBytes.str());

    // Deserialize the JSON into a list of subscription requests
    std::vector<RequestInformationType*> subscriptionVector =
            JsonSerializer::deserializeVector<RequestInformationType>(jsonBytes.str());

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

void PublicationManager::removeAttributePublication(const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger, "removePublication: {}", subscriptionId);

    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_DEBUG(logger, "publication {}  does not exist - will not remove", subscriptionId);
        return;
    }

    std::shared_ptr<Publication> publication(publications.take(subscriptionId));
    std::shared_ptr<SubscriptionRequestInformation> request(
            subscriptionId2SubscriptionRequest.take(subscriptionId));

    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2SubscriptionRequest));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Delete the onChange publication if needed
        removeOnChangePublication(subscriptionId, request, publication);
    }

    saveAttributeSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeBroadcastPublication(const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger, "removeBroadcast: {}", subscriptionId);

    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_DEBUG(logger, "publication {}  does not exist - will not remove", subscriptionId);
        return;
    }

    std::shared_ptr<Publication> publication = nullptr;
    if (publications.contains(subscriptionId)) {
        publication = std::shared_ptr<Publication>(publications.take(subscriptionId));
    }

    std::shared_ptr<BroadcastSubscriptionRequestInformation> request = nullptr;
    if (subscriptionId2BroadcastSubscriptionRequest.contains(subscriptionId)) {
        request = std::shared_ptr<BroadcastSubscriptionRequestInformation>(
                subscriptionId2BroadcastSubscriptionRequest.take(subscriptionId));
    }

    std::vector<Variant> subscriptionList(
            subscriptionMapToVectorCopy(subscriptionId2BroadcastSubscriptionRequest));

    if (publication != nullptr && request != nullptr) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Remove listener
        std::shared_ptr<RequestCaller> requestCaller = publication->requestCaller;
        requestCaller->unregisterBroadcastListener(
                request->getSubscribeToName(), publication->broadcastListener);
        publication->broadcastListener = nullptr;

        removePublicationEndRunnable(publication);
    }

    saveBroadcastSubscriptionRequestsMap(subscriptionList);
}

void PublicationManager::removeOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
    JOYNR_LOG_DEBUG(logger, "Removing onChange publication for id = {}", subscriptionId);
    // to silence unused-variable compiler warnings
    (void)subscriptionId;

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

// This function assumes that a read lock is already held
bool PublicationManager::processFilterChain(
        const std::string& subscriptionId,
        const std::vector<Variant>& broadcastValues,
        const std::vector<std::shared_ptr<IBroadcastFilter>>& filters)
{
    bool success = true;

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));
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

std::int64_t PublicationManager::getPublicationTtl(
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
    JOYNR_LOG_DEBUG(logger, "sending subscription error");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setError(exceptions::JoynrExceptionUtil::createVariant(exception));
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, subscriptionPublication);
    JOYNR_LOG_DEBUG(logger, "sent subscription error");
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
        removeAll(currentScheduledPublications, request->getSubscriptionId());
    }
    JOYNR_LOG_TRACE(logger, "sent publication @ {}", now);
}

void PublicationManager::sendPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        const std::vector<Variant>& value)
{
    JOYNR_LOG_DEBUG(logger, "sending subscription reply");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setResponse(value);
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
                Util::attributeGetterFromName(subscriptionRequest->getSubscribeToName()));
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
                JOYNR_LOG_DEBUG(
                        logger, "rescheduling runnable with delay: {}", publicationInterval);
                delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                           std::chrono::milliseconds(publicationInterval));
            }
        };

        std::function<void(const exceptions::JoynrException&)> onError =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const exceptions::JoynrException& exception) {

            sendPublicationError(publication, subscriptionRequest, subscriptionRequest, exception);

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                JOYNR_LOG_DEBUG(
                        logger, "rescheduling runnable with delay: {}", publicationInterval);
                delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                           std::chrono::milliseconds(publicationInterval));
            }
        };

        JOYNR_LOG_DEBUG(logger, "run: executing requestInterpreter= {}", attributeGetter);
        try {
            requestInterpreter->execute(requestCaller,
                                        attributeGetter,
                                        std::vector<Variant>(),
                                        std::vector<std::string>(),
                                        onSuccess,
                                        onError);
            // ApplicationException is not possible for attributes in Franca
        } catch (exceptions::ProviderRuntimeException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Could not perform pollSubscription, caught exception: {} : {}",
                            e.getTypeName(),
                            e.getMessage());
            onError(e);
        } catch (exceptions::JoynrRuntimeException& e) {
            JOYNR_LOG_ERROR(logger,
                            "Could not perform an pollSubscription, caught exception: {} : {}",
                            e.getTypeName(),
                            e.getMessage());
            onError(exceptions::ProviderRuntimeException("caught exception: " + e.getTypeName() +
                                                         ":" + e.getMessage()));
        }
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

void PublicationManager::attributeValueChanged(const std::string& subscriptionId,
                                               const Variant& value)
{
    JOYNR_LOG_DEBUG(logger, "attributeValueChanged for onChange subscription {}", subscriptionId);

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_ERROR(logger,
                        "attributeValueChanged called for non-existing subscription {}",
                        subscriptionId);
        return;
    }

    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2SubscriptionRequest.value(subscriptionId));

    std::shared_ptr<Publication> publication(publications.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        if (!isPublicationAlreadyScheduled(subscriptionId)) {
            std::int64_t timeUntilNextPublication =
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

void PublicationManager::broadcastOccurred(
        const std::string& subscriptionId,
        const std::vector<Variant>& values,
        const std::vector<std::shared_ptr<IBroadcastFilter>>& filters)
{
    JOYNR_LOG_DEBUG(logger,
                    "broadcastOccurred for subscription {}.  Number of values: ",
                    subscriptionId,
                    values.size());

    // See if the subscription is still valid
    if (!publicationExists(subscriptionId)) {
        JOYNR_LOG_ERROR(logger,
                        "broadcastOccurred called for non-existing subscription {}",
                        subscriptionId);
        return;
    }

    std::shared_ptr<BroadcastSubscriptionRequestInformation> subscriptionRequest(
            subscriptionId2BroadcastSubscriptionRequest.value(subscriptionId));
    std::shared_ptr<Publication> publication(publications.value(subscriptionId));

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->mutex));
        // Only proceed if publication can immediately be sent
        std::int64_t timeUntilNextPublication =
                getTimeUntilNextPublication(publication, subscriptionRequest->getQos());

        if (timeUntilNextPublication == 0) {
            // Execute broadcast filters
            if (processFilterChain(subscriptionId, values, filters)) {
                // Send the publication
                sendPublication(publication, subscriptionRequest, subscriptionRequest, values);
            }
        } else {
            if (timeUntilNextPublication > 0) {
                JOYNR_LOG_DEBUG(logger,
                                "Omitting broadcast publication for subscription {} because of too "
                                "short interval. Next publication possible in {} ms",
                                subscriptionId,
                                timeUntilNextPublication);
            } else {
                JOYNR_LOG_DEBUG(
                        logger,
                        "Omitting broadcast publication for subscription {} because of error.",
                        subscriptionId);
            }
        }
    }
}

bool PublicationManager::isPublicationAlreadyScheduled(const std::string& subscriptionId)
{
    std::lock_guard<std::mutex> currentScheduledLocker(currentScheduledPublicationsMutex);
    return vectorContains(currentScheduledPublications, subscriptionId);
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
        if (!vectorContains(currentScheduledPublications, subscriptionId)) {
            JOYNR_LOG_DEBUG(logger, "rescheduling runnable with delay: {}", nextPublication);
            currentScheduledPublications.push_back(subscriptionId);
            delayedScheduler->schedule(new PublisherRunnable(*this, subscriptionId),
                                       std::chrono::milliseconds(nextPublication));
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
    if (!publicationManager.publicationExists(subscriptionId)) {
        return;
    }
    std::shared_ptr<Publication> publication(publicationManager.publications.value(subscriptionId));
    publicationManager.removePublication(subscriptionId);

    {
        std::lock_guard<std::recursive_mutex> lock((publication->mutex));
        publication->publicationEndRunnableHandle = DelayedScheduler::INVALID_RUNNABLE_HANDLE;
    }
}

} // namespace joynr
