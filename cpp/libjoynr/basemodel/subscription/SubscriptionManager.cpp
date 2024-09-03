/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#include "joynr/SubscriptionManager.h"

#include <chrono>
#include <cstdint>
#include <mutex>

#include <boost/asio/io_service.hpp>

#include "joynr/IMessageRouter.h"
#include "joynr/ISubscriptionCallback.h"
#include "joynr/ISubscriptionListener.h"
#include "joynr/MulticastReceiverDirectory.h"
#include "joynr/MulticastSubscriptionRequest.h"
#include "joynr/SubscriptionQos.h"
#include "joynr/SubscriptionRequest.h"
#include "joynr/SubscriptionUtil.h"
#include "joynr/ThreadPoolDelayedScheduler.h"
#include "joynr/Util.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/SubscriptionException.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

class SubscriptionManager::Subscription
{
public:
    explicit Subscription(std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
                          std::shared_ptr<ISubscriptionListenerBase> subscriptionListener);
    ~Subscription() = default;

    std::int64_t _timeOfLastPublication;
    std::shared_ptr<ISubscriptionCallback> _subscriptionCaller;
    std::shared_ptr<ISubscriptionListenerBase> _subscriptionListener;
    std::recursive_mutex _mutex;
    bool _isStopped;
    std::uint32_t _subscriptionEndRunnableHandle;
    std::uint32_t _missedPublicationRunnableHandle;
    std::string _multicastId;
    std::string _subscriberParticipantId;
    std::string _providerParticipantId;

private:
    DISALLOW_COPY_AND_ASSIGN(Subscription);
};

SubscriptionManager::~SubscriptionManager()
{
    JOYNR_LOG_TRACE(logger(), "Destructing...");
    // check if all missed publication runnables are deleted before
    // deleting the missed publication scheduler

    _missedPublicationScheduler->shutdown();
    _subscriptions.deleteAll();
}

SubscriptionManager::SubscriptionManager(boost::asio::io_service& ioService,
                                         std::shared_ptr<IMessageRouter> messageRouter)
        : ISubscriptionManager(),
          enable_shared_from_this<SubscriptionManager>(),
          _subscriptions(),
          _multicastSubscribers(),
          _multicastSubscribersMutex(),
          _messageRouter(messageRouter),
          _missedPublicationScheduler(
                  std::make_shared<ThreadPoolDelayedScheduler>(1, "MissedPublications", ioService))
{
}

SubscriptionManager::SubscriptionManager(std::shared_ptr<DelayedScheduler> scheduler,
                                         std::shared_ptr<IMessageRouter> messageRouter)
        : ISubscriptionManager(),
          enable_shared_from_this<SubscriptionManager>(),
          _subscriptions(),
          _multicastSubscribers(),
          _multicastSubscribersMutex(),
          _messageRouter(messageRouter),
          _missedPublicationScheduler(scheduler)
{
}

void SubscriptionManager::shutdown()
{
    _missedPublicationScheduler->shutdown();
}

void SubscriptionManager::registerSubscription(
        const std::string& subscribeToName,
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
        std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
        std::shared_ptr<SubscriptionQos> qos,
        SubscriptionRequest& subscriptionRequest)
{
    // Register the subscription
    std::string subscriptionId = subscriptionRequest.getSubscriptionId();

    if (_subscriptions.contains(subscriptionId)) {
        // pre-existing subscription: remove it first from the internal data structure
        unregisterSubscription(subscriptionId);
    }

    const std::int64_t timeNow1 = std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count();

    const std::int64_t subscriptionExpiryDateMs = qos->getExpiryDateMs();
    const std::int64_t alertAfterInterval = SubscriptionUtil::getAlertInterval(qos);
    const std::int64_t periodicPublicationInterval =
            SubscriptionUtil::getPeriodicPublicationInterval(qos);

    subscriptionRequest.setQos(qos);

    if (subscriptionExpiryDateMs != SubscriptionQos::NO_EXPIRY_DATE() &&
        subscriptionExpiryDateMs < timeNow1) {
        throw std::invalid_argument("Subscription ExpiryDate " +
                                    std::to_string(subscriptionExpiryDateMs) +
                                    " in the past. Now: " + std::to_string(timeNow1));
    }

    auto subscription = std::make_shared<Subscription>(subscriptionCaller, subscriptionListener);

    _subscriptions.insert(subscriptionId, subscription);
    JOYNR_LOG_TRACE(logger(), "Subscription registered. ID={}", subscriptionId);

    {
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->_mutex);
        if (alertAfterInterval > 0 && periodicPublicationInterval > 0) {
            JOYNR_LOG_TRACE(logger(), "Will notify if updates are missed.");
            auto expiryDate = TimePoint::fromAbsoluteMs(subscriptionExpiryDateMs);
            if (subscriptionExpiryDateMs == SubscriptionQos::NO_EXPIRY_DATE()) {
                expiryDate = TimePoint::max();
            }
            subscription->_missedPublicationRunnableHandle = _missedPublicationScheduler->schedule(
                    std::make_shared<MissedPublicationRunnable>(expiryDate,
                                                                periodicPublicationInterval,
                                                                subscriptionId,
                                                                subscription,
                                                                shared_from_this(),
                                                                alertAfterInterval),
                    std::chrono::milliseconds(alertAfterInterval));
        } else if (subscriptionExpiryDateMs != SubscriptionQos::NO_EXPIRY_DATE()) {
            const std::int64_t timeNow2 =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch())
                            .count();

            subscription->_subscriptionEndRunnableHandle = _missedPublicationScheduler->schedule(
                    std::make_shared<SubscriptionEndRunnable>(subscriptionId, shared_from_this()),
                    std::chrono::milliseconds(subscriptionExpiryDateMs - timeNow2));
        }
    }
    subscriptionRequest.setSubscribeToName(subscribeToName);
}

void SubscriptionManager::registerSubscription(
        const std::string& subscribeToName,
        const std::string& subscriberParticipantId,
        const std::string& providerParticipantId,
        const std::vector<std::string>& partitions,
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
        std::shared_ptr<ISubscriptionListenerBase> subscriptionListener,
        std::shared_ptr<SubscriptionQos> qos,
        MulticastSubscriptionRequest& subscriptionRequest,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::string multicastId =
            util::createMulticastId(providerParticipantId, subscribeToName, partitions);
    subscriptionRequest.setMulticastId(multicastId);
    {
        std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(
                _multicastSubscribersMutex);
        std::string subscriptionId = subscriptionRequest.getSubscriptionId();

        JOYNR_LOG_DEBUG(logger(),
                        "MulticastSubscription: subscriptionId: {}, "
                        "proxy participantId: {}, provider participantId: {}",
                        subscriptionId,
                        subscriberParticipantId,
                        providerParticipantId);

        // remove pre-exisiting multicast subscription
        if (_subscriptions.contains(subscriptionId)) {
            std::shared_ptr<Subscription> subscription(_subscriptions.value(subscriptionId));
            assert(subscription);
            if (!subscription) {
                JOYNR_LOG_FATAL(logger(),
                                "registerSubscription: internal error, nullptr found for "
                                "subscription with id {}",
                                subscriptionId);
                return;
            }
            std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->_mutex);
            std::string oldMulticastId = subscription->_multicastId;
            if (multicastId != oldMulticastId) {
                unregisterSubscription(subscriptionId);
            } else {
                // do not call message router if multicast id has not changed
                _subscriptions.remove(subscriptionId);
                stopSubscription(subscription);
            }
        }

        JOYNR_LOG_DEBUG(logger(),
                        "SUBSCRIPTION call proxy: subscriptionId: {}, multicastId: {}, broadcast: "
                        "{}, qos: {}, "
                        "proxy participantId: {}, provider participantId: [{}]",
                        subscriptionId,
                        multicastId,
                        subscriptionRequest.getSubscribeToName(),
                        joynr::serializer::serializeToJson(*qos),
                        subscriberParticipantId,
                        providerParticipantId);
        // register multicast subscription
        registerSubscription(subscribeToName,
                             subscriptionCaller,
                             subscriptionListener,
                             qos,
                             subscriptionRequest);
        std::shared_ptr<Subscription> subscription(_subscriptions.value(subscriptionId));
        assert(subscription);
        if (!subscription) {
            JOYNR_LOG_FATAL(logger(),
                            "registerSubscription: internal error, nullptr found for subscription "
                            "with id {}",
                            subscriptionId);
            return;
        }
        {
            std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->_mutex);
            subscription->_multicastId = multicastId;
            subscription->_subscriberParticipantId = subscriberParticipantId;
            subscription->_providerParticipantId = providerParticipantId;
        }
        if (!_multicastSubscribers.contains(multicastId, subscriptionId)) {
            _messageRouter->addMulticastReceiver(multicastId,
                                                 subscriberParticipantId,
                                                 providerParticipantId,
                                                 std::move(onSuccess),
                                                 std::move(onError));
            _multicastSubscribers.registerMulticastReceiver(multicastId, subscriptionId);
        } else {
            if (onSuccess) {
                onSuccess();
            }
        }
    }
}

void SubscriptionManager::stopSubscription(std::shared_ptr<Subscription> subscription)
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->_mutex);
    subscription->_isStopped = true;
    if (subscription->_subscriptionEndRunnableHandle !=
        DelayedScheduler::_INVALID_RUNNABLE_HANDLE) {
        _missedPublicationScheduler->unschedule(subscription->_subscriptionEndRunnableHandle);
        subscription->_subscriptionEndRunnableHandle = DelayedScheduler::_INVALID_RUNNABLE_HANDLE;
    }
    if (subscription->_missedPublicationRunnableHandle !=
        DelayedScheduler::_INVALID_RUNNABLE_HANDLE) {
        _missedPublicationScheduler->unschedule(subscription->_missedPublicationRunnableHandle);
        subscription->_missedPublicationRunnableHandle = DelayedScheduler::_INVALID_RUNNABLE_HANDLE;
    }
}

void SubscriptionManager::unregisterSubscription(const std::string& subscriptionId)
{
    if (!_subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_TRACE(logger(),
                        "Called unregister on a non/no longer existent subscription, used id= {}",
                        subscriptionId);
        return;
    }
    std::shared_ptr<Subscription> subscription(_subscriptions.take(subscriptionId));
    assert(subscription);
    if (!subscription) {
        JOYNR_LOG_FATAL(
                logger(),
                "unregisterSubscription: internal error, nullptr found for subscription with id {}",
                subscriptionId);
        return;
    }

    std::unique_lock<std::recursive_mutex> multicastSubscribersLocker(_multicastSubscribersMutex);
    std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->_mutex);
    JOYNR_LOG_TRACE(
            logger(), "Called unregister / unsubscribe on subscription id= {}", subscriptionId);
    std::string multicastId = subscription->_multicastId;
    if (!multicastId.empty()) {
        stopSubscription(subscription);
        // remove multicast subscriber
        if (!_multicastSubscribers.unregisterMulticastReceiver(multicastId, subscriptionId)) {
            JOYNR_LOG_FATAL(logger(),
                            "No multicast subscriber found for subscriptionId={}, multicastId={}",
                            subscriptionId,
                            multicastId);
            return;
        }
        auto onSuccess = [subscriptionId, multicastId]() {
            JOYNR_LOG_TRACE(logger(),
                            "Multicast receiver unregistered. ID={}, multicastId={}",
                            subscriptionId,
                            multicastId);
        };
        std::shared_ptr<ISubscriptionListenerBase> subscriptionListener =
                subscription->_subscriptionListener;
        auto onError = [subscriptionId, multicastId, subscriptionListener](
                               const joynr::exceptions::ProviderRuntimeException& error) {
            std::string message =
                    "Unsubscribe from subscription (ID=" + subscriptionId +
                    ", multicastId=" + multicastId +
                    ") failed. Could not remove multicast receiver: " + error.getMessage();
            exceptions::SubscriptionException subscriptionException(message, subscriptionId);
            subscriptionListener->onError(subscriptionException);
        };
        _messageRouter->removeMulticastReceiver(multicastId,
                                                subscription->_subscriberParticipantId,
                                                subscription->_providerParticipantId,
                                                std::move(onSuccess),
                                                std::move(onError));
        return;
    }
    multicastSubscribersLocker.unlock();

    stopSubscription(subscription);
}

#if 0
void SubscriptionManager::checkMissedPublication(
    const Timer::TimerId id)
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(&mutex);

    if (!isExpired() && !subscription->isStopped)
    {
        JOYNR_LOG_TRACE(logger(), "Running MissedPublicationRunnable for subscription id= {}",subscriptionId);
        std::int64_t delay = 0;
        std::int64_t now = duration_cast<milliseconds>(
            system_clock::now().time_since_epoch()).count();
        std::int64_t timeSinceLastPublication = now
            - subscription->timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < alertAfterInterval;
        if (publicationInTime)
        {
            JOYNR_LOG_TRACE(logger(), "Publication in time!");
            delay = alertAfterInterval - timeSinceLastPublication;
        }
        else
        {
            JOYNR_LOG_TRACE(logger(), "Publication missed!");
            std::shared_ptr<ISubscriptionCallback> callback =
                subscription->subscriptionCaller;

            callback->onError();
            delay = alertAfterInterval
                - timeSinceLastExpectedPublication(timeSinceLastPublication);
        }
        JOYNR_LOG_TRACE(logger(), "Resceduling MissedPublicationRunnable with delay: {}",std::string::number(delay));
        subscription->missedPublicationRunnableHandle =
            subscriptionManager.missedPublicationScheduler->schedule(
                new MissedPublicationRunnable(decayTime,
                    expectedIntervalMSecs,
                    subscriptionId,
                    subscription,
                    subscriptionManager,
                    alertAfterInterval),
                delay);
    }
    else
    {
        JOYNR_LOG_TRACE(logger(), "Publication expired / interrupted. Expiring on subscription id={}",subscriptionId);
    }
}
#endif

void SubscriptionManager::touchSubscriptionState(const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(logger(), "Touching subscription state for id={}", subscriptionId);
    if (!_subscriptions.contains(subscriptionId)) {
        return;
    }
    std::shared_ptr<Subscription> subscription(_subscriptions.value(subscriptionId));
    if (!subscription) {
        JOYNR_LOG_ERROR(
                logger(),
                "touchSubscriptionState: internal error, nullptr found for subscription with id {}",
                subscriptionId);
        return;
    }
    {
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
        std::lock_guard<std::recursive_mutex> subscriptionLocker(subscription->_mutex);
        subscription->_timeOfLastPublication = now;
    }
}

std::shared_ptr<ISubscriptionCallback> SubscriptionManager::getSubscriptionCallback(
        const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(
            logger(), "Getting subscription callback for subscription id={}", subscriptionId);
    if (!_subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_TRACE(logger(),
                        "Trying to access a non existing subscription callback for id={}",
                        subscriptionId);
        return std::shared_ptr<ISubscriptionCallback>();
    }

    std::shared_ptr<Subscription> subscription(_subscriptions.value(subscriptionId));
    if (!subscription) {
        JOYNR_LOG_ERROR(logger(),
                        "getSubscriptionCallback: internal error, nullptr found for subscription "
                        "with id {}",
                        subscriptionId);
        return nullptr;
    }
    {
        std::lock_guard<std::recursive_mutex> subscriptionLockers(subscription->_mutex);
        return subscription->_subscriptionCaller;
    }
}

std::shared_ptr<ISubscriptionCallback> SubscriptionManager::getMulticastSubscriptionCallback(
        const std::string& multicastId)
{
    std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(_multicastSubscribersMutex);
    auto subscriptionIds = _multicastSubscribers.getReceivers(multicastId);
    if (subscriptionIds.empty()) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to access a non existing subscription callback for multicast id={}",
                       multicastId);
        return std::shared_ptr<ISubscriptionCallback>();
    }
    return getSubscriptionCallback(*(subscriptionIds.begin()));
}

std::shared_ptr<ISubscriptionListenerBase> SubscriptionManager::getSubscriptionListener(
        const std::string& subscriptionId)
{
    JOYNR_LOG_TRACE(
            logger(), "Getting subscription listener for subscription id={}", subscriptionId);
    if (!_subscriptions.contains(subscriptionId)) {
        JOYNR_LOG_WARN(logger(),
                       "Trying to access a non existing subscription listener for id={}",
                       subscriptionId);
        return std::shared_ptr<ISubscriptionListenerBase>();
    }

    std::shared_ptr<Subscription> subscription(_subscriptions.value(subscriptionId));
    assert(subscription);
    if (!subscription) {
        JOYNR_LOG_FATAL(logger(),
                        "getSubscriptionListener: internal error, nullptr found for subscription "
                        "with id {}",
                        subscriptionId);
        return nullptr;
    }
    {
        std::lock_guard<std::recursive_mutex> subscriptionLockers(subscription->_mutex);
        return subscription->_subscriptionListener;
    }
}

std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> SubscriptionManager::
        getMulticastSubscriptionListeners(const std::string& multicastId)
{
    std::forward_list<std::shared_ptr<ISubscriptionListenerBase>> listeners;
    {
        std::lock_guard<std::recursive_mutex> multicastSubscribersLocker(
                _multicastSubscribersMutex);
        auto subscriptionIds = _multicastSubscribers.getReceivers(multicastId);
        for (const auto& subscriptionId : subscriptionIds) {
            std::shared_ptr<ISubscriptionListenerBase> listener =
                    getSubscriptionListener(subscriptionId);
            if (listener) {
                listeners.push_front(listener);
            }
        }
    }
    return listeners;
}

//------ SubscriptionManager::Subscription ---------------------------------------
SubscriptionManager::Subscription::Subscription(
        std::shared_ptr<ISubscriptionCallback> subscriptionCaller,
        std::shared_ptr<ISubscriptionListenerBase> subscriptionListener)
        : _timeOfLastPublication(0),
          _subscriptionCaller(subscriptionCaller),
          _subscriptionListener(subscriptionListener),
          _mutex(),
          _isStopped(false),
          _subscriptionEndRunnableHandle(),
          _missedPublicationRunnableHandle(),
          _multicastId(),
          _subscriberParticipantId(),
          _providerParticipantId()
{
}

/**
 *  SubscriptionManager::MissedPublicationRunnable
 */
SubscriptionManager::MissedPublicationRunnable::MissedPublicationRunnable(
        const TimePoint& expiryDate,
        std::int64_t expectedIntervalMSecs,
        const std::string& subscriptionId,
        std::shared_ptr<Subscription> subscription,
        std::weak_ptr<SubscriptionManager> subscriptionManager,
        std::int64_t alertAfterInterval)
        : Runnable(),
          ObjectWithDecayTime(expiryDate),
          _expectedIntervalMSecs(expectedIntervalMSecs),
          _subscription(subscription),
          _subscriptionId(subscriptionId),
          _alertAfterInterval(alertAfterInterval),
          _subscriptionManager(subscriptionManager)
{
}

void SubscriptionManager::MissedPublicationRunnable::shutdown()
{
}

void SubscriptionManager::MissedPublicationRunnable::run()
{
    std::lock_guard<std::recursive_mutex> subscriptionLocker(_subscription->_mutex);

    if (!isExpired() && !_subscription->_isStopped) {
        std::int64_t delay = 0;
        std::int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                                   std::chrono::system_clock::now().time_since_epoch())
                                   .count();
        std::int64_t timeSinceLastPublication = now - _subscription->_timeOfLastPublication;
        bool publicationInTime = timeSinceLastPublication < _alertAfterInterval;
        if (publicationInTime) {
            JOYNR_LOG_TRACE(
                    logger(), "Publication in time for subscription id={}", _subscriptionId);
            delay = _alertAfterInterval - timeSinceLastPublication;
        } else {
            JOYNR_LOG_TRACE(logger(), "Publication missed for subscription id={}", _subscriptionId);
            std::shared_ptr<ISubscriptionListenerBase> listener =
                    _subscription->_subscriptionListener;

            exceptions::PublicationMissedException error(_subscriptionId);
            listener->onError(error);
            delay = _alertAfterInterval -
                    timeSinceLastExpectedPublication(timeSinceLastPublication);
        }

        if (auto subscriptionManagerSharedPtr = _subscriptionManager.lock()) {
            JOYNR_LOG_TRACE(
                    logger(), "Rescheduling MissedPublicationRunnable with delay: {}", delay);
            _subscription->_missedPublicationRunnableHandle =
                    subscriptionManagerSharedPtr->_missedPublicationScheduler->schedule(
                            std::make_shared<MissedPublicationRunnable>(_decayTime,
                                                                        _expectedIntervalMSecs,
                                                                        _subscriptionId,
                                                                        _subscription,
                                                                        _subscriptionManager,
                                                                        _alertAfterInterval),
                            std::chrono::milliseconds(delay));
        } else {
            JOYNR_LOG_ERROR(logger(),
                            "Error: Failed to rescheduling MissedPublicationRunnable with delay: "
                            "{} because SubscriptionManager is not available",
                            delay);
        }
    } else {
        JOYNR_LOG_TRACE(logger(),
                        "Publication expired / interrupted. Expiring on subscription id={}",
                        _subscriptionId);
    }
}

std::int64_t SubscriptionManager::MissedPublicationRunnable::timeSinceLastExpectedPublication(
        std::int64_t timeSinceLastPublication)
{
    return timeSinceLastPublication % _expectedIntervalMSecs;
}

SubscriptionManager::SubscriptionEndRunnable::SubscriptionEndRunnable(
        const std::string& subscriptionId,
        std::weak_ptr<SubscriptionManager> subscriptionManager)
        : Runnable(), _subscriptionId(subscriptionId), _subscriptionManager(subscriptionManager)
{
}

void SubscriptionManager::SubscriptionEndRunnable::shutdown()
{
}

void SubscriptionManager::SubscriptionEndRunnable::run()
{
    if (auto subscriptionManagerSharedPtr = _subscriptionManager.lock()) {
        JOYNR_LOG_TRACE(logger(),
                        "Running SubscriptionEndRunnable for subscription id= {}",
                        _subscriptionId);
        JOYNR_LOG_TRACE(logger(),
                        "Publication expired / interrupted. Expiring on subscription id={}",
                        _subscriptionId);
        subscriptionManagerSharedPtr->unregisterSubscription(_subscriptionId);
    } else {
        JOYNR_LOG_ERROR(logger(),
                        "Error running SubscriptionEndRunnable for subscription id= {} because "
                        "subscriptionManager is not available",
                        _subscriptionId);
    }
}

} // namespace joynr
