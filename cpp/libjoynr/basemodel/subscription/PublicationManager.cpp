/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <cassert>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <sstream>

#include "joynr/BroadcastSubscriptionRequest.h"
#include "joynr/CallContextStorage.h"
#include "joynr/DelayedScheduler.h"
#include "joynr/IPublicationSender.h"
#include "joynr/IRequestInterpreter.h"
#include "joynr/InterfaceRegistrar.h"
#include "joynr/MessagingQos.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/Reply.h"
#include "joynr/Request.h"
#include "joynr/RequestCaller.h"
#include "joynr/Runnable.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/TimePoint.h"
#include "joynr/UnicastSubscriptionQos.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/JoynrExceptionUtil.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class PublicationManager::PublisherRunnable : public Runnable
{
public:
    ~PublisherRunnable() override = default;
    PublisherRunnable(std::weak_ptr<PublicationManager> publicationManager,
                      const std::string& subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::pollSubscription()
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(PublisherRunnable);
    std::weak_ptr<PublicationManager> _publicationManager;
    std::string _subscriptionId;
};

class PublicationManager::PublicationEndRunnable : public Runnable
{
public:
    ~PublicationEndRunnable() override = default;
    PublicationEndRunnable(std::weak_ptr<PublicationManager> _publicationManager,
                           const std::string& _subscriptionId);

    void shutdown() override;

    // Calls PublicationManager::removePublication()
    void run() override;

private:
    DISALLOW_COPY_AND_ASSIGN(PublicationEndRunnable);
    std::weak_ptr<PublicationManager> _publicationManager;
    std::string _subscriptionId;
};

//------ PublicationManager ----------------------------------------------------

PublicationManager::~PublicationManager()
{
    std::lock_guard<std::mutex> shutDownLocker(_shutDownMutex);
    assert(_shuttingDown);
}

void PublicationManager::shutdown()
{
    {
        std::lock_guard<std::mutex> shutDownLocker(_shutDownMutex);
        assert(!_shuttingDown);
        if (_shuttingDown) {
            return;
        }
        _shuttingDown = true;
    }

    JOYNR_LOG_TRACE(logger(), "shutting down thread pool and scheduler ...");
    _delayedScheduler->shutdown();

    // Remove all publications
    JOYNR_LOG_TRACE(logger(), "removing publications");

    while (_subscriptionId2SubscriptionRequest.size() > 0) {
        auto subscriptionRequest = _subscriptionId2SubscriptionRequest.begin();
        removeAttributePublication((subscriptionRequest->second)->getSubscriptionId());
    }

    while (_subscriptionId2BroadcastSubscriptionRequest.size() > 0) {
        auto broadcastRequest = _subscriptionId2BroadcastSubscriptionRequest.begin();
        removeBroadcastPublication((broadcastRequest->second)->getSubscriptionId());
    }
}

PublicationManager::PublicationManager(boost::asio::io_service& ioService,
                                       std::weak_ptr<IMessageSender> messageSender,
                                       std::uint64_t ttlUplift,
                                       int maxThreads)
        : _messageSender(messageSender),
          _publications(),
          _subscriptionId2SubscriptionRequest(),
          _subscriptionId2BroadcastSubscriptionRequest(),
          _fileWriteLock(),
          _delayedScheduler(std::make_shared<ThreadPoolDelayedScheduler>(maxThreads,
                                                                         "PubManager",
                                                                         ioService)),
          _shutDownMutex(),
          _shuttingDown(false),
          _queuedSubscriptionRequests(),
          _queuedSubscriptionRequestsMutex(),
          _queuedBroadcastSubscriptionRequests(),
          _queuedBroadcastSubscriptionRequestsMutex(),
          _currentScheduledPublications(),
          _currentScheduledPublicationsMutex(),
          _broadcastFilterLock(),
          _ttlUplift(ttlUplift),
          _publicationsMutex()
{
}

bool isSubscriptionExpired(const std::shared_ptr<SubscriptionQos> qos, int offset = 0)
{
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    return qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE() &&
           qos->getExpiryDateMs() < (now + offset);
}

void PublicationManager::sendSubscriptionReply(std::weak_ptr<IPublicationSender> publicationSender,
                                               const std::string& fromParticipantId,
                                               const std::string& toParticipantId,
                                               std::int64_t expiryDateMs,
                                               const SubscriptionReply& subscriptionReply)
{
    MessagingQos messagingQos;
    if (expiryDateMs == SubscriptionQos::NO_EXPIRY_DATE()) {
        messagingQos.setTtl(INT64_MAX);
    } else {
        const auto timePoint = TimePoint::fromAbsoluteMs(expiryDateMs);
        messagingQos.setTtl(static_cast<std::uint64_t>(timePoint.relativeFromNow().count()));
    }
    if (auto publicationSenderSharedPtr = publicationSender.lock()) {
        publicationSenderSharedPtr->sendSubscriptionReply(
                fromParticipantId, toParticipantId, messagingQos, subscriptionReply);
    }
}

void PublicationManager::sendSubscriptionReply(std::weak_ptr<IPublicationSender> publicationSender,
                                               const std::string& fromParticipantId,
                                               const std::string& toParticipantId,
                                               std::int64_t expiryDateMs,
                                               const std::string& subscriptionId)
{
    SubscriptionReply subscriptionReply;
    subscriptionReply.setSubscriptionId(subscriptionId);
    sendSubscriptionReply(
            publicationSender, fromParticipantId, toParticipantId, expiryDateMs, subscriptionReply);
}

void PublicationManager::sendSubscriptionReply(
        std::weak_ptr<IPublicationSender> publicationSender,
        const std::string& fromParticipantId,
        const std::string& toParticipantId,
        std::int64_t expiryDateMs,
        const std::string& subscriptionId,
        std::shared_ptr<exceptions::SubscriptionException> error)
{
    SubscriptionReply subscriptionReply;
    subscriptionReply.setSubscriptionId(subscriptionId);
    subscriptionReply.setError(std::move(error));
    sendSubscriptionReply(
            publicationSender, fromParticipantId, toParticipantId, expiryDateMs, subscriptionReply);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             SubscriptionRequest& subscriptionRequest,
                             std::weak_ptr<IPublicationSender> publicationSender)
{
    assert(requestCaller);
    auto requestInfo = std::make_shared<SubscriptionRequestInformation>(proxyParticipantId,
                                                                        providerParticipantId,
                                                                        CallContextStorage::get(),
                                                                        subscriptionRequest);
    handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::addSubscriptionCleanupIfNecessary(std::shared_ptr<Publication> publication,
                                                           std::shared_ptr<SubscriptionQos> qos,
                                                           const std::string& subscriptionId)
{
    if (qos->getExpiryDateMs() != SubscriptionQos::NO_EXPIRY_DATE()) {

        std::chrono::hours tolerance = std::chrono::hours(1);
        const TimePoint max = TimePoint::max() - tolerance;
        TimePoint publicationEnd;
        if (qos->getExpiryDateMs() > max.toMilliseconds() - static_cast<std::int64_t>(_ttlUplift)) {
            publicationEnd = max;
        } else {
            publicationEnd = TimePoint::fromAbsoluteMs(qos->getExpiryDateMs() +
                                                       static_cast<std::int64_t>(_ttlUplift));
        }
        publication->_publicationEndRunnableHandle = _delayedScheduler->schedule(
                std::make_shared<PublicationEndRunnable>(shared_from_this(), subscriptionId),
                publicationEnd.relativeFromNow());
        JOYNR_LOG_TRACE(logger(),
                        "publication will end in {}  ms",
                        publicationEnd.relativeFromNow().count());
    }
}

void PublicationManager::handleAttributeSubscriptionRequest(
        std::shared_ptr<SubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        std::weak_ptr<IPublicationSender> publicationSender)
{
    const std::string& subscriptionId = requestInfo->getSubscriptionId();
    auto publication = std::make_shared<Publication>(publicationSender, requestCaller);

    if (publicationExists(subscriptionId)) {
        JOYNR_LOG_TRACE(logger(),
                        "Publication with id: {}  already exists. Updating...",
                        requestInfo->getSubscriptionId());
        removeAttributePublication(subscriptionId);
    }

    _subscriptionId2SubscriptionRequest.insert(subscriptionId, requestInfo);
    // Make note of the publication
    _publications.insert(subscriptionId, publication);

    JOYNR_LOG_DEBUG(logger(), "added subscription: {}", requestInfo->toString());

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
        // Add an onChange publication if needed
        addOnChangePublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const std::shared_ptr<SubscriptionQos> qos = requestInfo->getQos();

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            addSubscriptionCleanupIfNecessary(publication, qos, subscriptionId);
            {
                std::lock_guard<std::mutex> currentScheduledLocker(
                        _currentScheduledPublicationsMutex);
                _currentScheduledPublications.push_back(subscriptionId);
            }
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  qos->getExpiryDateMs(),
                                  subscriptionId);
            // sent at least once the current value
            _delayedScheduler->schedule(
                    std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId));
        } else {
            JOYNR_LOG_WARN(logger(), "publication end is in the past");
            const TimePoint expiryDate =
                    TimePoint::fromRelativeMs(60000) + static_cast<std::int64_t>(_ttlUplift);
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  expiryDate.toMilliseconds(),
                                  subscriptionId,
                                  std::make_shared<exceptions::SubscriptionException>(
                                          "publication end is in the past", subscriptionId));
        }
    }
}

void PublicationManager::addOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        JOYNR_LOG_TRACE(logger(), "adding onChange subscription: {}", subscriptionId);

        // Create an attribute listener to listen for onChange events
        std::shared_ptr<SubscriptionAttributeListener> attributeListener =
                std::make_shared<SubscriptionAttributeListener>(subscriptionId, shared_from_this());

        // Register the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->_requestCaller;
        requestCaller->registerAttributeListener(request->getSubscribeToName(), attributeListener);

        // Make note of the attribute listener so that it can be unregistered
        publication->_attributeListener = std::move(attributeListener);
    }
}

void PublicationManager::addBroadcastPublication(
        const std::string& subscriptionId,
        std::shared_ptr<BroadcastSubscriptionRequestInformation> request,
        std::shared_ptr<PublicationManager::Publication> publication)
{
    JOYNR_LOG_TRACE(logger(), "adding broadcast subscription: {}", subscriptionId);

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));

    // Create a broadcast listener to listen for broadcast events
    std::shared_ptr<UnicastBroadcastListener> broadcastListener =
            std::make_shared<UnicastBroadcastListener>(subscriptionId, shared_from_this());

    // Register the broadcast listener
    std::shared_ptr<RequestCaller> requestCaller = publication->_requestCaller;
    requestCaller->registerBroadcastListener(request->getSubscribeToName(), broadcastListener);

    // Make note of the attribute listener so that it can be unregistered
    publication->_broadcastListener = std::move(broadcastListener);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             SubscriptionRequest& subscriptionRequest)
{
    JOYNR_LOG_TRACE(logger(),
                    "Added subscription for non existing provider (adding subscriptionRequest "
                    "to queue).");
    auto requestInfo = std::make_shared<SubscriptionRequestInformation>(proxyParticipantId,
                                                                        providerParticipantId,
                                                                        CallContextStorage::get(),
                                                                        subscriptionRequest);
    {
        std::lock_guard<std::mutex> queueLocker(_queuedSubscriptionRequestsMutex);
        _queuedSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId(), requestInfo));
    }

    _subscriptionId2SubscriptionRequest.insert(requestInfo->getSubscriptionId(), requestInfo);
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             MulticastSubscriptionRequest& subscriptionRequest,
                             std::weak_ptr<IPublicationSender> publicationSender)
{
    // silently handle multicast subscription request
    sendSubscriptionReply(publicationSender,
                          providerParticipantId,
                          proxyParticipantId,
                          subscriptionRequest.getQos()->getExpiryDateMs(),
                          subscriptionRequest.getSubscriptionId());
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             std::shared_ptr<RequestCaller> requestCaller,
                             BroadcastSubscriptionRequest& subscriptionRequest,
                             std::weak_ptr<IPublicationSender> publicationSender)
{
    assert(requestCaller);
    auto requestInfo = std::make_shared<BroadcastSubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);

    handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
}

void PublicationManager::handleBroadcastSubscriptionRequest(
        std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo,
        std::shared_ptr<RequestCaller> requestCaller,
        std::weak_ptr<IPublicationSender> publicationSender)
{
    const std::string& subscriptionId = requestInfo->getSubscriptionId();

    auto publication = std::make_shared<Publication>(publicationSender, requestCaller);

    if (publicationExists(subscriptionId)) {
        JOYNR_LOG_TRACE(logger(),
                        "Publication with id: {}  already exists. Updating...",
                        requestInfo->getSubscriptionId());
        removeBroadcastPublication(subscriptionId);
    }

    _subscriptionId2BroadcastSubscriptionRequest.insert(subscriptionId, requestInfo);

    // Make note of the publication
    _publications.insert(subscriptionId, publication);
    JOYNR_LOG_DEBUG(logger(), "added subscription: {}", requestInfo->toString());

    {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
        // Add an onChange publication if needed
        addBroadcastPublication(subscriptionId, requestInfo, publication);

        // Schedule a runnable to remove the publication when it finishes
        const std::shared_ptr<SubscriptionQos> qos = requestInfo->getQos();

        // check for a valid publication end date
        if (!isSubscriptionExpired(qos)) {
            addSubscriptionCleanupIfNecessary(publication, qos, subscriptionId);
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  qos->getExpiryDateMs(),
                                  subscriptionId);
        } else {
            JOYNR_LOG_WARN(logger(), "publication end is in the past");
            const TimePoint expiryDate =
                    TimePoint::fromRelativeMs(60000) + static_cast<std::int64_t>(_ttlUplift);
            sendSubscriptionReply(publicationSender,
                                  requestInfo->getProviderId(),
                                  requestInfo->getProxyId(),
                                  expiryDate.toMilliseconds(),
                                  subscriptionId,
                                  std::make_shared<exceptions::SubscriptionException>(
                                          "publication end is in the past", subscriptionId));
        }
    }
}

void PublicationManager::add(const std::string& proxyParticipantId,
                             const std::string& providerParticipantId,
                             BroadcastSubscriptionRequest& subscriptionRequest)
{
    JOYNR_LOG_TRACE(logger(),
                    "Added broadcast subscription for non existing provider (adding "
                    "subscriptionRequest to queue).");
    auto requestInfo = std::make_shared<BroadcastSubscriptionRequestInformation>(
            proxyParticipantId, providerParticipantId, subscriptionRequest);
    {
        std::lock_guard<std::mutex> queueLocker(_queuedBroadcastSubscriptionRequestsMutex);
        _queuedBroadcastSubscriptionRequests.insert(
                std::make_pair(requestInfo->getProviderId(), requestInfo));
    }

    _subscriptionId2BroadcastSubscriptionRequest.insert(
            requestInfo->getSubscriptionId(), requestInfo);
}

void PublicationManager::removeAllSubscriptions(const std::string& providerId)
{
    JOYNR_LOG_TRACE(logger(), "Removing all subscriptions for provider id= {}", providerId);

    // Build lists of subscriptionIds to remove
    std::vector<std::string> publicationsToRemove;
    {
        auto callback = [&publicationsToRemove, &providerId](auto&& mapParam) {
            for (auto&& requestInfo : mapParam) {
                std::string subscriptionId = requestInfo.second->getSubscriptionId();

                if ((requestInfo.second)->getProviderId() == providerId) {
                    publicationsToRemove.push_back(subscriptionId);
                }
            }
        };
        _subscriptionId2SubscriptionRequest.applyReadFun(callback);
    }

    std::vector<std::string> broadcastsToRemove;
    {
        auto callback = [&broadcastsToRemove, &providerId](auto&& mapParam) {
            for (auto& requestInfo : mapParam) {
                std::string subscriptionId = requestInfo.second->getSubscriptionId();

                if ((requestInfo.second)->getProviderId() == providerId) {
                    broadcastsToRemove.push_back(subscriptionId);
                }
            }
        };
        _subscriptionId2BroadcastSubscriptionRequest.applyReadFun(callback);
    }

    // Remove each publication
    for (const std::string& subscriptionId : publicationsToRemove) {
        JOYNR_LOG_TRACE(logger(),
                        "Removing subscription providerId= {}, subscriptionId = {}",
                        providerId,
                        subscriptionId);
        removeAttributePublication(subscriptionId);
    }

    // Remove each broadcast
    for (const std::string& subscriptionId : broadcastsToRemove) {
        JOYNR_LOG_TRACE(logger(),
                        "Removing subscription providerId= {}, subscriptionId = {}",
                        providerId,
                        subscriptionId);
        removeBroadcastPublication(subscriptionId);
    }
}

void PublicationManager::stopPublication(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger(), "stopPublication: {}", subscriptionId);
    removePublication(subscriptionId);
}

bool PublicationManager::publicationExists(const std::string& subscriptionId) const
{
    return _publications.contains(subscriptionId);
}

void PublicationManager::restore(const std::string& providerId,
                                 std::shared_ptr<RequestCaller> requestCaller,
                                 std::weak_ptr<IPublicationSender> publicationSender)
{
    JOYNR_LOG_TRACE(logger(), "restore: entering ...");

    {
        std::lock_guard<std::mutex> queueLocker(_queuedSubscriptionRequestsMutex);
        std::multimap<std::string, std::shared_ptr<SubscriptionRequestInformation>>::iterator
                queuedSubscriptionRequestsIterator = _queuedSubscriptionRequests.find(providerId);
        while (queuedSubscriptionRequestsIterator != _queuedSubscriptionRequests.end()) {
            std::shared_ptr<SubscriptionRequestInformation> requestInfo(
                    queuedSubscriptionRequestsIterator->second);
            _queuedSubscriptionRequests.erase(queuedSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                JOYNR_LOG_TRACE(logger(),
                                "Restoring subscription for provider: {} {}",
                                providerId,
                                requestInfo->toString());
                handleAttributeSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedSubscriptionRequestsIterator = _queuedSubscriptionRequests.find(providerId);
        }
    }

    {
        std::lock_guard<std::mutex> queueLocker(_queuedBroadcastSubscriptionRequestsMutex);
        std::multimap<std::string,
                      std::shared_ptr<BroadcastSubscriptionRequestInformation>>::iterator
                queuedBroadcastSubscriptionRequestsIterator =
                        _queuedBroadcastSubscriptionRequests.find(providerId);
        while (queuedBroadcastSubscriptionRequestsIterator !=
               _queuedBroadcastSubscriptionRequests.end()) {
            std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo(
                    queuedBroadcastSubscriptionRequestsIterator->second);
            _queuedBroadcastSubscriptionRequests.erase(queuedBroadcastSubscriptionRequestsIterator);
            if (!isSubscriptionExpired(requestInfo->getQos())) {
                JOYNR_LOG_TRACE(logger(),
                                "Restoring subscription for provider: {}  {}",
                                providerId,
                                requestInfo->toString());
                handleBroadcastSubscriptionRequest(requestInfo, requestCaller, publicationSender);
            }
            queuedBroadcastSubscriptionRequestsIterator =
                    _queuedBroadcastSubscriptionRequests.find(providerId);
        }
    }
}

void PublicationManager::removeAttributePublication(const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger(), "removePublication: {}", subscriptionId);

    std::unique_lock<std::mutex> publicationsLock(_publicationsMutex);
    std::shared_ptr<Publication> publication = _publications.take(subscriptionId);
    publicationsLock.unlock();
    std::shared_ptr<SubscriptionRequestInformation> request =
            _subscriptionId2SubscriptionRequest.take(subscriptionId);

    if (publication && request) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
        // Delete the onChange publication if needed
        removeOnChangePublication(subscriptionId, request, publication);
    }
}

void PublicationManager::removeBroadcastPublication(const std::string& subscriptionId)
{
    JOYNR_LOG_DEBUG(logger(), "removeBroadcast: {}", subscriptionId);

    std::unique_lock<std::mutex> publicationsLock(_publicationsMutex);
    std::shared_ptr<Publication> publication = _publications.take(subscriptionId);
    publicationsLock.unlock();

    std::shared_ptr<BroadcastSubscriptionRequestInformation> request =
            _subscriptionId2BroadcastSubscriptionRequest.take(subscriptionId);

    if (publication && request) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
        // Remove listener
        std::shared_ptr<RequestCaller> requestCaller = publication->_requestCaller;
        requestCaller->unregisterBroadcastListener(
                request->getSubscribeToName(), publication->_broadcastListener);
        publication->_broadcastListener = nullptr;

        removePublicationEndRunnable(publication);
    }
}

void PublicationManager::removeOnChangePublication(
        const std::string& subscriptionId,
        std::shared_ptr<SubscriptionRequestInformation> request,
        std::shared_ptr<Publication> publication)
{
    assert(request);
    assert(publication);
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
    JOYNR_LOG_TRACE(logger(), "Removing onChange publication for id = {}", subscriptionId);
    // to silence unused-variable compiler warnings
    std::ignore = subscriptionId;

    if (SubscriptionUtil::isOnChangeSubscription(request->getQos())) {
        // Unregister and delete the attribute listener
        std::shared_ptr<RequestCaller> requestCaller = publication->_requestCaller;
        requestCaller->unregisterAttributeListener(
                request->getSubscribeToName(), publication->_attributeListener);
        publication->_attributeListener = nullptr;
    }
    removePublicationEndRunnable(publication);
}

// This function assumes a write lock is alrady held for the publication}
void PublicationManager::removePublicationEndRunnable(std::shared_ptr<Publication> publication)
{
    assert(publication);
    if (publication->_publicationEndRunnableHandle != DelayedScheduler::_INVALID_RUNNABLE_HANDLE &&
        !isShuttingDown()) {
        JOYNR_LOG_TRACE(logger(),
                        "Unscheduling PublicationEndRunnable with handle: {}",
                        publication->_publicationEndRunnableHandle);
        _delayedScheduler->unschedule(publication->_publicationEndRunnableHandle);
        publication->_publicationEndRunnableHandle = DelayedScheduler::_INVALID_RUNNABLE_HANDLE;
    }
}

bool PublicationManager::isShuttingDown()
{
    std::lock_guard<std::mutex> shuwDownLocker(_shutDownMutex);
    return _shuttingDown;
}

std::int64_t PublicationManager::getPublicationTtlMs(
        std::shared_ptr<SubscriptionRequest> subscriptionRequest) const
{
    assert(subscriptionRequest);
    // Get publication ttl only if subscriptionQos is a UnicastSubscritpionQos
    auto qos = subscriptionRequest->getQos();
    if (auto unicastQos = std::dynamic_pointer_cast<UnicastSubscriptionQos>(qos)) {
        return unicastQos->getPublicationTtlMs();
    }
    JOYNR_LOG_WARN(
            logger(), "Attempted to get publication ttl out of an invalid Qos: returing default.");
    return UnicastSubscriptionQos::DEFAULT_PUBLICATION_TTL_MS();
}

void PublicationManager::sendPublicationError(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        std::shared_ptr<exceptions::JoynrRuntimeException> exception)
{
    assert(publication);
    assert(subscriptionInformation);
    assert(request);
    assert(exception);
    JOYNR_LOG_TRACE(logger(), "sending subscription error");
    SubscriptionPublication subscriptionPublication;
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    subscriptionPublication.setError(std::move(exception));
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, std::move(subscriptionPublication));
    JOYNR_LOG_TRACE(logger(), "sent subscription error");
}

void PublicationManager::sendSubscriptionPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        SubscriptionPublication&& subscriptionPublication)
{
    assert(publication);
    assert(subscriptionInformation);
    assert(request);

    MessagingQos mQos;

    std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
    // Set the TTL
    mQos.setTtl(static_cast<std::uint64_t>(getPublicationTtlMs(request)));

    std::weak_ptr<IPublicationSender> publicationSender = publication->_sender;

    if (auto publicationSenderSharedPtr = publicationSender.lock()) {
        publicationSenderSharedPtr->sendSubscriptionPublication(
                subscriptionInformation->getProviderId(),
                subscriptionInformation->getProxyId(),
                mQos,
                std::move(subscriptionPublication));
        // Make note of when this publication was sent
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
        publication->_timeOfLastPublication = now;

        {
            std::lock_guard<std::mutex> currentScheduledLocker(_currentScheduledPublicationsMutex);
            util::removeAll(_currentScheduledPublications, request->getSubscriptionId());
        }
        JOYNR_LOG_TRACE(logger(), "sent publication @ {}", now);
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "publication could not be sent because publicationSender is not available");
    }
}

void PublicationManager::sendPublication(
        std::shared_ptr<Publication> publication,
        std::shared_ptr<SubscriptionInformation> subscriptionInformation,
        std::shared_ptr<SubscriptionRequest> request,
        BaseReply&& response)
{
    assert(publication);
    assert(subscriptionInformation);
    assert(request);

    JOYNR_LOG_TRACE(logger(), "sending subscription reply");
    SubscriptionPublication subscriptionPublication(std::move(response));
    subscriptionPublication.setSubscriptionId(request->getSubscriptionId());
    sendSubscriptionPublication(
            publication, subscriptionInformation, request, std::move(subscriptionPublication));
    JOYNR_LOG_TRACE(logger(), "sent subscription reply");
}

void PublicationManager::pollSubscription(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger(), "pollSubscription {}", subscriptionId);

    if (isShuttingDown()) {
        return;
    }

    // Get the subscription details
    std::unique_lock<std::mutex> publicationsLock(_publicationsMutex);
    std::shared_ptr<Publication> publication = _publications.value(subscriptionId);
    std::shared_ptr<SubscriptionRequestInformation> subscriptionRequest =
            _subscriptionId2SubscriptionRequest.value(subscriptionId);

    if (publication && subscriptionRequest) {
        std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
        publicationsLock.unlock();
        // See if the publication is needed
        const std::shared_ptr<SubscriptionQos> qos = subscriptionRequest->getQos();
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
        std::int64_t publicationInterval = SubscriptionUtil::getPeriodicPublicationInterval(qos);

        // check if the subscription qos needs a periodic publication
        if (publicationInterval > 0) {
            std::int64_t timeSinceLast = now - publication->_timeOfLastPublication;
            // publish only if not published in the current interval
            if (timeSinceLast < publicationInterval) {
                JOYNR_LOG_TRACE(
                        logger(),
                        "no publication necessary. publicationInterval: {}, timeSinceLast {}",
                        publicationInterval,
                        timeSinceLast);

                std::int64_t delayUntilNextPublication = publicationInterval - timeSinceLast;
                assert(delayUntilNextPublication >= 0);
                _delayedScheduler->schedule(
                        std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                        std::chrono::milliseconds(delayUntilNextPublication));
                return;
            }
        }

        std::shared_ptr<RequestCaller> requestCaller = publication->_requestCaller;
        const std::string& interfaceName = requestCaller->getInterfaceName();
        std::shared_ptr<IRequestInterpreter> requestInterpreter =
                InterfaceRegistrar::instance().getRequestInterpreter(
                        interfaceName +
                        std::to_string(requestCaller->getProviderVersion().getMajorVersion()));
        if (!requestInterpreter) {
            JOYNR_LOG_ERROR(
                    logger(),
                    "requestInterpreter not found for interface {} while polling subscriptionId {}",
                    interfaceName,
                    subscriptionId);
            return;
        }

        // Get the value of the attribute
        std::string attributeGetter(
                util::attributeGetterFromName(subscriptionRequest->getSubscribeToName()));

        std::function<void(Reply &&)> onSuccess = [publication,
                                                   publicationInterval,
                                                   qos,
                                                   subscriptionRequest,
                                                   this,
                                                   subscriptionId](Reply&& response) {
            sendPublication(
                    publication, subscriptionRequest, subscriptionRequest, std::move(response));

            // Reschedule the next poll
            if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                JOYNR_LOG_TRACE(
                        logger(), "rescheduling runnable with delay: {}", publicationInterval);
                _delayedScheduler->schedule(
                        std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                        std::chrono::milliseconds(publicationInterval));
            }
        };

        std::function<void(const std::shared_ptr<exceptions::JoynrException>&)> onError =
                [publication, publicationInterval, qos, subscriptionRequest, this, subscriptionId](
                        const std::shared_ptr<exceptions::JoynrException>& exception) {
                    std::shared_ptr<exceptions::JoynrRuntimeException> runtimeError =
                            std::dynamic_pointer_cast<exceptions::JoynrRuntimeException>(exception);
                    assert(runtimeError);
                    sendPublicationError(publication,
                                         subscriptionRequest,
                                         subscriptionRequest,
                                         std::move(runtimeError));

                    // Reschedule the next poll
                    if (publicationInterval > 0 && (!isSubscriptionExpired(qos))) {
                        JOYNR_LOG_TRACE(logger(),
                                        "rescheduling runnable with delay: {}",
                                        publicationInterval);
                        _delayedScheduler->schedule(std::make_shared<PublisherRunnable>(
                                                            shared_from_this(), subscriptionId),
                                                    std::chrono::milliseconds(publicationInterval));
                    }
                };

        JOYNR_LOG_TRACE(logger(), "run: executing requestInterpreter= {}", attributeGetter);
        Request dummyRequest;
        dummyRequest.setMethodName(attributeGetter);

        CallContextStorage::set(subscriptionRequest->getCallContext());
        requestInterpreter->execute(
                std::move(requestCaller), dummyRequest, std::move(onSuccess), std::move(onError));
        CallContextStorage::invalidate();
    }
}

void PublicationManager::removePublication(const std::string& subscriptionId)
{
    if (_subscriptionId2SubscriptionRequest.contains(subscriptionId)) {
        removeAttributePublication(subscriptionId);
    } else if (_subscriptionId2BroadcastSubscriptionRequest.contains(subscriptionId)) {
        removeBroadcastPublication(subscriptionId);
    }
}

bool PublicationManager::isPublicationAlreadyScheduled(const std::string& subscriptionId)
{
    std::lock_guard<std::mutex> currentScheduledLocker(_currentScheduledPublicationsMutex);
    return util::vectorContains(_currentScheduledPublications, subscriptionId);
}

std::int64_t PublicationManager::getTimeUntilNextPublication(
        std::shared_ptr<Publication> publication,
        const std::shared_ptr<SubscriptionQos> qos)
{
    std::lock_guard<std::recursive_mutex> publicationLocker((publication->_mutex));
    // Check the last publication time against the min interval
    std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count();
    std::int64_t minInterval = SubscriptionUtil::getMinInterval(qos);

    std::int64_t timeSinceLast = now - publication->_timeOfLastPublication;

    if (minInterval > 0 && timeSinceLast < minInterval) {
        return minInterval - timeSinceLast;
    }

    return 0;
}

void PublicationManager::reschedulePublication(const std::string& subscriptionId,
                                               std::int64_t nextPublication)
{
    if (nextPublication > 0) {
        std::lock_guard<std::mutex> currentScheduledLocker(_currentScheduledPublicationsMutex);

        // Schedule a publication so that the change is not forgotten
        if (!util::vectorContains(_currentScheduledPublications, subscriptionId)) {
            JOYNR_LOG_TRACE(logger(), "rescheduling runnable with delay: {}", nextPublication);
            _currentScheduledPublications.push_back(subscriptionId);
            _delayedScheduler->schedule(
                    std::make_shared<PublisherRunnable>(shared_from_this(), subscriptionId),
                    std::chrono::milliseconds(nextPublication));
        }
    }
}

//------ PublicationManager::Publication ---------------------------------------

PublicationManager::Publication::Publication(std::weak_ptr<IPublicationSender> publicationSender,
                                             std::shared_ptr<RequestCaller> requestCaller)
        : _timeOfLastPublication(0),
          _sender(publicationSender),
          _requestCaller(std::move(requestCaller)),
          _attributeListener(nullptr),
          _broadcastListener(nullptr),
          _mutex(),
          _publicationEndRunnableHandle(DelayedScheduler::_INVALID_RUNNABLE_HANDLE)
{
}

//------ PublicationManager::PublisherRunnable ---------------------------------

PublicationManager::PublisherRunnable::PublisherRunnable(
        std::weak_ptr<PublicationManager> publicationManager,
        const std::string& subscriptionId)
        : Runnable(),
          _publicationManager(std::move(publicationManager)),
          _subscriptionId(subscriptionId)
{
}

void PublicationManager::PublisherRunnable::shutdown()
{
}

void PublicationManager::PublisherRunnable::run()
{
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        publicationManagerSharedPtr->pollSubscription(_subscriptionId);
    }
}

//------ PublicationManager::PublicationEndRunnable ----------------------------

PublicationManager::PublicationEndRunnable::PublicationEndRunnable(
        std::weak_ptr<PublicationManager> publicationManager,
        const std::string& subscriptionId)
        : Runnable(), _publicationManager(publicationManager), _subscriptionId(subscriptionId)
{
}

void PublicationManager::PublicationEndRunnable::shutdown()
{
}

void PublicationManager::PublicationEndRunnable::run()
{
    if (auto publicationManagerSharedPtr = _publicationManager.lock()) {
        std::unique_lock<std::mutex> publicationsLock(
                publicationManagerSharedPtr->_publicationsMutex);
        std::shared_ptr<Publication> publication =
                publicationManagerSharedPtr->_publications.value(_subscriptionId);
        if (publication) {
            std::lock_guard<std::recursive_mutex> lock((publication->_mutex));
            publication->_publicationEndRunnableHandle = DelayedScheduler::_INVALID_RUNNABLE_HANDLE;
        }
        publicationsLock.unlock();
        // publicationsMutex is acquired again in next call
        publicationManagerSharedPtr->removePublication(_subscriptionId);
    }
}

} // namespace joynr
