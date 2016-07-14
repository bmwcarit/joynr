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
#ifndef PUBLICATIONMANAGER_H
#define PUBLICATIONMANAGER_H
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/SubscriptionPublication.h"

#include "joynr/Logger.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/ReadWriteLock.h"
#include "joynr/ThreadSafeMap.h"

#include <mutex>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <boost/asio/io_service.hpp>

#include "joynr/Variant.h"

namespace joynr
{

class SubscriptionRequest;
class BroadcastSubscriptionRequest;
class BroadcastSubscriptionRequestInformation;
class SubscriptionRequestInformation;
class SubscriptionInformation;
class IPublicationSender;
class RequestCaller;
class IBroadcastFilter;

namespace exceptions
{
class JoynrException;
} // namespace exceptions
class SubscriptionQos;

/**
  * @class PublicationManager
  * @brief Publication manager receives subscription requests and prepares publications,
  * which are send back to the subscription manager.
  * Responsible for deleting SubscriptionRequests and PublicationStates (the runnable notifies the
  * SubscriptionManager when it terminates - this triggeres the delete).
  */
class JOYNR_EXPORT PublicationManager
{
public:
    explicit PublicationManager(boost::asio::io_service& ioService, int maxThreads = 2);
    explicit PublicationManager(DelayedScheduler* scheduler);
    virtual ~PublicationManager();
    /**
     * @brief Adds the SubscriptionRequest and starts runnable to poll attributes.
     * @param requestCaller
     * @param subscriptionRequest
     * @param publicationSender
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             std::shared_ptr<RequestCaller> requestCaller,
             SubscriptionRequest& subscriptionRequest,
             IPublicationSender* publicationSender);

    /**
     * @brief Adds SubscriptionRequest when the Provider is not yet registered
     *   and there is no RequestCaller as yet.
     *
     * @param subscriptionRequest
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             SubscriptionRequest& subscriptionRequest);

    /**
     * @brief Adds the BroadcastSubscriptionRequest and starts runnable to poll attributes.
     * @param requestCaller
     * @param subscriptionRequest
     * @param publicationSender
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             std::shared_ptr<RequestCaller> requestCaller,
             BroadcastSubscriptionRequest& subscriptionRequest,
             IPublicationSender* publicationSender);

    /**
     * @brief Adds BroadcastSubscriptionRequest when the Provider is not yet registered
     *   and there is no RequestCaller as yet.
     *
     * @param subscriptionRequest
     */
    void add(const std::string& proxyParticipantId,
             const std::string& providerParticipantId,
             BroadcastSubscriptionRequest& subscriptionRequest);

    /**
     * @brief Stops the sending of publications
     *
     * @param subscriptionId
     */
    void stopPublication(const std::string& subscriptionId);

    /**
     * @brief Stops all publications for a provider
     *
     * @param providerId
     */
    void removeAllSubscriptions(const std::string& providerId);

    /**
     * @brief Called by the Dispatcher every time a provider is registered to check whether there
     * are already subscriptionRequests waiting.
     *
     * @param providerId
     * @param requestCaller
     * @param publicationSender
     */
    void restore(const std::string& providerId,
                 std::shared_ptr<RequestCaller> requestCaller,
                 IPublicationSender* publicationSender);

    /**
      * @brief Publishes an onChange message when an attribute value changes
      *
      * This method is virtual so that it can be overridden by a mock object.
      * @param subscriptionId A subscription that was listening on the attribute
      * @param value The new attribute value
      */
    virtual void attributeValueChanged(const std::string& subscriptionId, const Variant& value);

    /**
      * @brief Publishes an broadcast publication message when a broadcast occurs
      *
      * This method is virtual so that it can be overridden by a mock object.
      * @param subscriptionId A subscription that was listening on the broadcast
      * @param values The new broadcast values
      */
    virtual void broadcastOccurred(const std::string& subscriptionId,
                                   const std::vector<Variant>& values,
                                   const std::vector<std::shared_ptr<IBroadcastFilter>>& filters);

    void loadSavedBroadcastSubscriptionRequestsMap(const std::string& fileName);
    void loadSavedAttributeSubscriptionRequestsMap(const std::string& fileName);

private:
    DISALLOW_COPY_AND_ASSIGN(PublicationManager);

    // A class that groups together the information needed for a publication
    class Publication;

    // Information for each publication is keyed by subcriptionId
    ThreadSafeMap<std::string, std::shared_ptr<Publication>> publications;
    ThreadSafeMap<std::string, std::shared_ptr<SubscriptionRequestInformation>>
            subscriptionId2SubscriptionRequest;
    ThreadSafeMap<std::string, std::shared_ptr<BroadcastSubscriptionRequestInformation>>
            subscriptionId2BroadcastSubscriptionRequest;

    std::mutex fileWriteLock;
    // Publications are scheduled to run on a thread pool
    DelayedScheduler* delayedScheduler;

    // Support for clean shutdowns
    std::mutex shutDownMutex;
    bool shuttingDown;

    // Subscription persistence
    std::string subscriptionRequestStorageFileName;
    std::string broadcastSubscriptionRequestStorageFileName;

    // Queues all subscription requests that are either received by the
    // dispatcher or restored from the subscription storage file before
    // the corresponding provider is added
    std::multimap<std::string, std::shared_ptr<SubscriptionRequestInformation>>
            queuedSubscriptionRequests;
    std::mutex queuedSubscriptionRequestsMutex;

    // Queues all broadcast subscription requests that are either received by the
    // dispatcher or restored from the subscription storage file before
    // the corresponding provider is added
    std::multimap<std::string, std::shared_ptr<BroadcastSubscriptionRequestInformation>>
            queuedBroadcastSubscriptionRequests;
    std::mutex queuedBroadcastSubscriptionRequestsMutex;

    // Logging
    ADD_LOGGER(PublicationManager);

    // List of subscriptionId's of runnables scheduled with delay <= qos.getMinIntervalMs_ms()
    std::vector<std::string> currentScheduledPublications;
    std::mutex currentScheduledPublicationsMutex;

    // Filters registered for broadcasts. Keyed by broadcast name.
    std::map<std::string, std::vector<std::shared_ptr<IBroadcastFilter>>> broadcastFilters;

    // Read/write lock for broadcast filters
    mutable ReadWriteLock broadcastFilterLock;

    // PublisherRunnables are used to send publications via a ThreadPool
    class PublisherRunnable;

    // PublicationEndRunnables finish a publication
    class PublicationEndRunnable;

    // Functions called by runnables
    void pollSubscription(const std::string& subscriptionId);
    void removePublication(const std::string& subscriptionId);
    void removeAttributePublication(const std::string& subscriptionId,
                                    const bool updatePersistenceFile = true);
    void removeBroadcastPublication(const std::string& subscriptionId,
                                    const bool updatePersistenceFile = true);

    // Helper functions
    bool publicationExists(const std::string& subscriptionId) const;
    void createPublishRunnable(const std::string& subscriptionId);
    void saveAttributeSubscriptionRequestsMap(const std::vector<Variant>& subscriptionList);
    void saveBroadcastSubscriptionRequestsMap(const std::vector<Variant>& subscriptionList);

    void reschedulePublication(const std::string& subscriptionId, std::int64_t nextPublication);

    bool isPublicationAlreadyScheduled(const std::string& subscriptionId);

    /**
     * @brief getTimeUntilNextPublication determines the time to wait until the next publication
     * can be sent base on the QOS information.
     * @param subscriptionId
     * @param qos
     * @return  0 if publication can immediately be sent;
     *          amount of ms to wait, if interval was too short;
     *          -1 on error
     */
    std::int64_t getTimeUntilNextPublication(std::shared_ptr<Publication> publication, Variant qos);

    void saveSubscriptionRequestsMap(const std::vector<Variant>& subscriptionList,
                                     const std::string& storageFilename);

    template <class RequestInformationType>
    void loadSavedSubscriptionRequestsMap(
            const std::string& storageFilename,
            std::mutex& mutex,
            std::multimap<std::string, std::shared_ptr<RequestInformationType>>&
                    queuedSubscriptions);

    template <class RequestInformationType>
    std::vector<Variant> subscriptionMapToVectorCopy(
            const ThreadSafeMap<std::string, std::shared_ptr<RequestInformationType>>& map);

    bool isShuttingDown();
    std::int64_t getPublicationTtlMs(
            std::shared_ptr<SubscriptionRequest> subscriptionRequest) const;
    void sendPublication(std::shared_ptr<Publication> publication,
                         std::shared_ptr<SubscriptionInformation> subscriptionInformation,
                         std::shared_ptr<SubscriptionRequest> subscriptionRequest,
                         const std::vector<Variant>& value);
    void sendSubscriptionPublication(
            std::shared_ptr<Publication> publication,
            std::shared_ptr<SubscriptionInformation> subscriptionInformation,
            std::shared_ptr<SubscriptionRequest> request,
            SubscriptionPublication& subscriptionPublication);
    void sendPublicationError(std::shared_ptr<Publication> publication,
                              std::shared_ptr<SubscriptionInformation> subscriptionInformation,
                              std::shared_ptr<SubscriptionRequest> subscriptionRequest,
                              const exceptions::JoynrException& exception);
    void handleAttributeSubscriptionRequest(
            std::shared_ptr<SubscriptionRequestInformation> requestInfo,
            std::shared_ptr<RequestCaller> requestCaller,
            IPublicationSender* publicationSender);

    void handleBroadcastSubscriptionRequest(
            std::shared_ptr<BroadcastSubscriptionRequestInformation> requestInfo,
            std::shared_ptr<RequestCaller> requestCaller,
            IPublicationSender* publicationSender);

    void addOnChangePublication(const std::string& subscriptionId,
                                std::shared_ptr<SubscriptionRequestInformation> request,
                                std::shared_ptr<Publication> publication);
    void addBroadcastPublication(const std::string& subscriptionId,
                                 std::shared_ptr<BroadcastSubscriptionRequestInformation> request,
                                 std::shared_ptr<Publication> publication);
    void removeOnChangePublication(const std::string& subscriptionId,
                                   std::shared_ptr<SubscriptionRequestInformation> request,
                                   std::shared_ptr<Publication> publication);
    void removePublicationEndRunnable(std::shared_ptr<Publication> publication);

    bool processFilterChain(const std::string& subscriptionId,
                            const std::vector<Variant>& broadcastValues,
                            const std::vector<std::shared_ptr<IBroadcastFilter>>& filters);
};

} // namespace joynr

#endif // PUBLICATIONMANAGER_H
