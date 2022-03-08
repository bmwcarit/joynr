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

/*
 * Client for the global capabilities directory.
 */

#include <atomic>
#include <mutex>
#include <boost/algorithm/string/join.hpp>

#include "GlobalCapabilitiesDirectoryClient.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Message.h"
#include "joynr/Semaphore.h"
#include "joynr/TimePoint.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/LCDUtil.h"
#include "joynr/types/GlobalDiscoveryEntry.h"

namespace joynr
{

GlobalCapabilitiesDirectoryClient::GlobalCapabilitiesDirectoryClient(
        const ClusterControllerSettings& clusterControllerSettings,
        std::unique_ptr<TaskSequencer<void>> taskSequencer)
        : _capabilitiesProxy(nullptr),
          _messagingQos(),
          _touchTtl(static_cast<std::uint64_t>(
                  clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs().count())),
          _removeStaleTtl(60000),
          _sequentialTasks(std::move(taskSequencer))
{
}

GlobalCapabilitiesDirectoryClient::~GlobalCapabilitiesDirectoryClient()
{
    shutdown();
}

void GlobalCapabilitiesDirectoryClient::shutdown()
{
    // Assure that all captures to class members are released.
    _sequentialTasks->cancel();
}

void GlobalCapabilitiesDirectoryClient::add(
        const types::GlobalDiscoveryEntry& entry,
        const bool awaitGlobalRegistration,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos addMessagingQos = _messagingQos;
    addMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    using std::move;
    TaskSequencer<void>::TaskWithExpiryDate addTask;

    if (!awaitGlobalRegistration) {
        addTask._expiryDate = TimePoint::max();
        addTask._timeout = []() {};
    } else {
        addTask._expiryDate =
                TimePoint::fromRelativeMs(static_cast<std::int64_t>(addMessagingQos.getTtl()));
        addTask._timeout = [onRuntimeError]() {
            onRuntimeError(exceptions::JoynrRuntimeException(
                    "Failed to process global registration in time, please try again"));
        };
    }

    auto addOperation = std::make_shared<AddOperation>(_capabilitiesProxy,
                                                       entry,
                                                       awaitGlobalRegistration,
                                                       gbids,
                                                       std::move(onSuccess),
                                                       std::move(onError),
                                                       std::move(onRuntimeError),
                                                       std::move(addMessagingQos),
                                                       addTask._expiryDate);

    addTask._task = [addOperation]() {
        addOperation->execute();
        return addOperation;
    };

    JOYNR_LOG_DEBUG(logger(),
                    "Global provider registration scheduled: participantId {}, domain {}, "
                    "interface {}, {}, awaitGlobalRegistration {}",
                    entry.getParticipantId(),
                    entry.getDomain(),
                    entry.getInterfaceName(),
                    entry.getProviderVersion().toString(),
                    awaitGlobalRegistration);
    _sequentialTasks->add(addTask);
}

void GlobalCapabilitiesDirectoryClient::reAdd(
        std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
        const std::string& localAddress)
{
    MessagingQos addMessagingQos = _messagingQos;

    TaskSequencer<void>::TaskWithExpiryDate reAddTask;
    reAddTask._expiryDate = TimePoint::max();
    reAddTask._timeout = []() {};
    reAddTask._task = [
        this,
        localCapabilitiesDirectoryStore,
        localAddress,
        addMessagingQos = std::move(addMessagingQos)
    ]() mutable
    {
        JOYNR_LOG_DEBUG(logger(), "Re-Add started.");
        std::shared_ptr<Future<void>> reAddResultFuture = std::make_shared<Future<void>>();
        std::vector<types::DiscoveryEntry> discoveryEntries;
        discoveryEntries = localCapabilitiesDirectoryStore->getAllGlobalCapabilities();
        if (discoveryEntries.empty()) {
            JOYNR_LOG_DEBUG(logger(), "Re-Add: no globally registered providers found.");
            reAddResultFuture->onSuccess();
            return reAddResultFuture;
        }

        std::shared_ptr<std::atomic_size_t> reAddCounter =
                std::make_shared<std::atomic_size_t>(discoveryEntries.size());

        auto onAddCompleted = [reAddCounter, reAddResultFuture]() {
            if (reAddCounter->fetch_sub(1) == 1) {
                JOYNR_LOG_INFO(logger(), "Re-Add completed.");
                reAddResultFuture->onSuccess();
            }
        };

        for (const auto& discoveryEntry : discoveryEntries) {
            const std::string participantId = discoveryEntry.getParticipantId();
            std::unique_lock<std::recursive_mutex> cacheLock(
                    localCapabilitiesDirectoryStore->getCacheLock());
            std::vector<std::string> gbids =
                    localCapabilitiesDirectoryStore->getGbidsForParticipantId(
                            participantId, cacheLock);
            cacheLock.unlock();
            if (gbids.empty()) {
                JOYNR_LOG_WARN(logger(), "Re-Add: no GBIDs found for {}", participantId);
                onAddCompleted();
                continue;
            }

            types::GlobalDiscoveryEntry globalDiscoveryEntry =
                    LCDUtil::toGlobalDiscoveryEntry(discoveryEntry, localAddress);

            std::shared_ptr<std::once_flag> onceFlag = std::make_shared<std::once_flag>();
            auto onSuccess = [onceFlag, onAddCompleted, participantId]() {
                JOYNR_LOG_INFO(logger(), "Re-Add succeeded for {}.", participantId);
                std::call_once(*onceFlag, onAddCompleted);
            };

            auto onError = [onceFlag, onAddCompleted, participantId](
                    const types::DiscoveryError::Enum& error) {
                JOYNR_LOG_ERROR(logger(),
                                "Re-Add failed for {} with error. Error: {}",
                                participantId,
                                types::DiscoveryError::getLiteral(error));
                std::call_once(*onceFlag, onAddCompleted);
            };

            auto onRuntimeError = [onceFlag, onAddCompleted, participantId](
                    const exceptions::JoynrRuntimeException& error) {
                JOYNR_LOG_ERROR(logger(),
                                "Re-Add failed for {} with exception: {} ({})",
                                participantId,
                                error.getMessage(),
                                error.getTypeName());
                std::call_once(*onceFlag, onAddCompleted);
            };

            addMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);

            _capabilitiesProxy->addAsync(globalDiscoveryEntry,
                                         std::move(gbids),
                                         std::move(onSuccess),
                                         std::move(onError),
                                         std::move(onRuntimeError),
                                         addMessagingQos);
        }
        return reAddResultFuture;
    };

    JOYNR_LOG_DEBUG(logger(), "Re-Add scheduled.");
    _sequentialTasks->add(reAddTask);
}

void GlobalCapabilitiesDirectoryClient::remove(
        const std::string& participantId,
        std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos removeMessagingQos = _messagingQos;
    auto retryRemoveOperation =
            std::make_shared<RetryRemoveOperation>(_capabilitiesProxy,
                                                   participantId,
                                                   localCapabilitiesDirectoryStore,
                                                   std::move(onSuccess),
                                                   std::move(onError),
                                                   std::move(onRuntimeError),
                                                   std::move(removeMessagingQos));
    TaskSequencer<void>::TaskWithExpiryDate removeTask;
    removeTask._expiryDate = TimePoint::max();
    removeTask._timeout = []() {};
    removeTask._task = [retryRemoveOperation]() {
        retryRemoveOperation->execute();
        return retryRemoveOperation;
    };

    JOYNR_LOG_DEBUG(logger(), "Global remove scheduled, participantId {}", participantId);
    _sequentialTasks->add(removeTask);
}

void GlobalCapabilitiesDirectoryClient::lookup(
        const std::vector<std::string>& domains,
        const std::string& interfaceName,
        const std::vector<std::string>& gbids,
        std::int64_t messagingTtl,
        std::function<void(const std::vector<types::GlobalDiscoveryEntry>& result)> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos lookupMessagingQos = _messagingQos;
    lookupMessagingQos.setTtl(static_cast<std::uint64_t>(messagingTtl));
    lookupMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    _capabilitiesProxy->lookupAsync(domains,
                                    interfaceName,
                                    std::move(gbids),
                                    std::move(onSuccess),
                                    std::move(onError),
                                    std::move(onRuntimeError),
                                    lookupMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::lookup(
        const std::string& participantId,
        const std::vector<std::string>& gbids,
        std::int64_t messagingTtl,
        std::function<void(const std::vector<joynr::types::GlobalDiscoveryEntry>& result)>
                onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos lookupMessagingQos = _messagingQos;
    lookupMessagingQos.setTtl(static_cast<std::uint64_t>(messagingTtl));
    lookupMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    _capabilitiesProxy->lookupAsync(participantId,
                                    std::move(gbids),
                                    [onSuccess = std::move(onSuccess)](
                                            const joynr::types::GlobalDiscoveryEntry& capability) {
                                        std::vector<joynr::types::GlobalDiscoveryEntry> result;
                                        result.push_back(capability);
                                        onSuccess(result);
                                    },
                                    std::move(onError),
                                    std::move(onRuntimeError),
                                    lookupMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::touch(
        const std::string& clusterControllerId,
        const std::vector<std::string>& participantIds,
        const std::string& gbid,
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::JoynrRuntimeException& error)> onError)
{
    MessagingQos touchMessagingQos = _messagingQos;
    touchMessagingQos.setTtl(_touchTtl);
    touchMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbid);
    _capabilitiesProxy->touchAsync(clusterControllerId,
                                   participantIds,
                                   std::move(onSuccess),
                                   std::move(onError),
                                   touchMessagingQos);
}

void GlobalCapabilitiesDirectoryClient::setProxy(
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy)
{
    this->_capabilitiesProxy = std::move(capabilitiesProxy);
}

void GlobalCapabilitiesDirectoryClient::removeStale(
        const std::string& clusterControllerId,
        std::int64_t maxLastSeenDateMs,
        const std::string gbid,
        std::function<void()> onSuccess,
        std::function<void(const exceptions::JoynrRuntimeException&)> onRuntimeError)
{
    MessagingQos removeStaleMessagingQos = _messagingQos;
    removeStaleMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbid);
    removeStaleMessagingQos.setTtl(_removeStaleTtl);
    _capabilitiesProxy->removeStaleAsync(clusterControllerId,
                                         maxLastSeenDateMs,
                                         std::move(onSuccess),
                                         std::move(onRuntimeError),
                                         removeStaleMessagingQos);
}

GlobalCapabilitiesDirectoryClient::RetryRemoveOperation::RetryRemoveOperation(
        const std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy>& capabilitiesProxy,
        const std::string& participantId,
        std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore,
        std::function<void()>&& onSuccessFunc,
        std::function<void(const types::DiscoveryError::Enum&)>&& onApplicationErrorFunc,
        std::function<void(const exceptions::JoynrRuntimeException&)>&& onRuntimeErrorFunc,
        MessagingQos qos)
        : Future<void>(),
          _capabilitiesProxy{capabilitiesProxy},
          _participantId{participantId},
          _localCapabilitiesDirectoryStore{localCapabilitiesDirectoryStore},
          _onSuccess{onSuccessFunc},
          _onApplicationError{onApplicationErrorFunc},
          _onRuntimeError{onRuntimeErrorFunc},
          _qos{qos}
{
}

void GlobalCapabilitiesDirectoryClient::RetryRemoveOperation::execute()
{
    if (StatusCodeEnum::IN_PROGRESS == getStatus()) {
        std::shared_ptr<LocalCapabilitiesDirectoryStore> localCapabilitiesDirectoryStore =
                _localCapabilitiesDirectoryStore.lock();
        if (localCapabilitiesDirectoryStore) {
            std::unique_lock<std::recursive_mutex> cacheLock(
                    localCapabilitiesDirectoryStore->getCacheLock());
            auto foundGbids = localCapabilitiesDirectoryStore->getGbidsForParticipantId(
                    _participantId, cacheLock);
            cacheLock.unlock();
            if (!foundGbids.empty()) {
                _qos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), foundGbids[0]);
                std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy>
                        capabilitiesProxy = _capabilitiesProxy.lock();
                if (capabilitiesProxy) {
                    using std::placeholders::_1;
                    JOYNR_LOG_INFO(logger(),
                                   "Removing globally registered participantId {} for GBIDs {}",
                                   _participantId,
                                   boost::algorithm::join(foundGbids, ","));
                    capabilitiesProxy->removeAsync(
                            _participantId,
                            foundGbids,
                            std::bind(&RetryRemoveOperation::forwardSuccess, shared_from_this()),
                            std::bind(&RetryRemoveOperation::forwardApplicationError,
                                      shared_from_this(),
                                      _1),
                            std::bind(&RetryRemoveOperation::retryOrForwardRuntimeError,
                                      shared_from_this(),
                                      _1),
                            _qos);
                } else {
                    const exceptions::JoynrRuntimeException proxyNotAvailable(
                            "Remove operation retry aborted since proxy not available.");
                    if (_onRuntimeError) {
                        _onRuntimeError(proxyNotAvailable);
                    }
                    onError(std::make_shared<exceptions::JoynrRuntimeException>(proxyNotAvailable));
                }
            } else {
                std::string errorMessage = "Global remove failed because participantId to GBIDs "
                                           "mapping is missing for participantId " +
                                           _participantId;
                JOYNR_LOG_WARN(logger(), errorMessage);
                exceptions::ProviderRuntimeException exception(errorMessage);
                onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
            }
        } else {
            std::string errorMessage = "Global remove failed of entry with participantId {} since "
                                       "localCapabilitiesDirectoryStore is not available.  " +
                                       _participantId;
            JOYNR_LOG_WARN(logger(), errorMessage);
            exceptions::ProviderRuntimeException exception(errorMessage);
            onError(std::make_shared<exceptions::JoynrRuntimeException>(exception));
        }
    } else {
        if (_onRuntimeError) {
            _onRuntimeError(exceptions::JoynrRuntimeException("Remove operation retry canceled."));
        }
    }
}

void GlobalCapabilitiesDirectoryClient::RetryRemoveOperation::forwardSuccess()
{
    if (_onSuccess) {
        _onSuccess();
    }
    onSuccess();
}

void GlobalCapabilitiesDirectoryClient::RetryRemoveOperation::forwardApplicationError(
        const types::DiscoveryError::Enum& e)
{
    if (_onApplicationError) {
        _onApplicationError(e);
    }
    onError(std::make_shared<exceptions::JoynrRuntimeException>(
            "Stop remove operation due to ApplicationException."));
}

void GlobalCapabilitiesDirectoryClient::RetryRemoveOperation::retryOrForwardRuntimeError(
        const exceptions::JoynrRuntimeException& e)
{
    if (typeid(exceptions::JoynrTimeOutException) == typeid(e)) {
        execute();
    } else {
        if (_onRuntimeError) {
            _onRuntimeError(e);
        }
        onError(std::make_shared<exceptions::JoynrRuntimeException>(e));
    }
}

GlobalCapabilitiesDirectoryClient::AddOperation::AddOperation(
        const std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy>& capabilitiesProxy,
        const types::GlobalDiscoveryEntry& entry,
        bool awaitGlobalRegistration,
        const std::vector<std::string>& gbids,
        std::function<void()>&& onSuccessFunc,
        std::function<void(const types::DiscoveryError::Enum&)>&& onApplicationErrorFunc,
        std::function<void(const exceptions::JoynrRuntimeException&)>&& onRuntimeErrorFunc,
        MessagingQos qos,
        const TimePoint& taskExpiryDate)
        : Future<void>(),
          _capabilitiesProxy{capabilitiesProxy},
          _globalDiscoveryEntry{entry},
          _awaitGlobalRegistration{awaitGlobalRegistration},
          _gbids{gbids},
          _onSuccess{onSuccessFunc},
          _onApplicationError{onApplicationErrorFunc},
          _onRuntimeError{onRuntimeErrorFunc},
          _qos{qos},
          _taskExpiryDate{taskExpiryDate}
{
    // Do nothing
}

void GlobalCapabilitiesDirectoryClient::AddOperation::execute()
{
    if (StatusCodeEnum::IN_PROGRESS == getStatus()) {
        if (_awaitGlobalRegistration) {
            std::int64_t remainingAddTtl = _taskExpiryDate.relativeFromNow().count();
            if (_taskExpiryDate.toMilliseconds() <= TimePoint::now().toMilliseconds()) {
                const exceptions::JoynrRuntimeException globalRegistrationFailed(
                        "Failed to process global registration in time, please try again");
                if (_onRuntimeError) {
                    _onRuntimeError(globalRegistrationFailed);
                }
                onSuccess();
                return;
            }
            _qos.setTtl(static_cast<std::uint64_t>(remainingAddTtl));
        }

        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy =
                _capabilitiesProxy.lock();
        if (capabilitiesProxy) {
            using std::placeholders::_1;
            JOYNR_LOG_DEBUG(logger(),
                            "Global provider registration started: participantId {}, domain {}, "
                            "interface {}, {}, awaitGlobalRegistration {}",
                            _globalDiscoveryEntry.getParticipantId(),
                            _globalDiscoveryEntry.getDomain(),
                            _globalDiscoveryEntry.getInterfaceName(),
                            _globalDiscoveryEntry.getProviderVersion().toString(),
                            _awaitGlobalRegistration);
            capabilitiesProxy->addAsync(
                    _globalDiscoveryEntry,
                    _gbids,
                    std::bind(&AddOperation::forwardSuccess, shared_from_this()),
                    std::bind(&AddOperation::forwardApplicationError, shared_from_this(), _1),
                    std::bind(&AddOperation::retryOrForwardRuntimeError, shared_from_this(), _1),
                    _qos);
        } else {
            const exceptions::JoynrRuntimeException proxyNotAvailable(
                    "Add operation retry aborted since proxy not available.");
            if (_onRuntimeError) {
                _onRuntimeError(proxyNotAvailable);
            }
            onError(std::make_shared<exceptions::JoynrRuntimeException>(proxyNotAvailable));
        }
    } else {
        if (_onRuntimeError) {
            _onRuntimeError(exceptions::JoynrRuntimeException("Add operation retry canceled."));
        }
    }
}

void GlobalCapabilitiesDirectoryClient::AddOperation::forwardSuccess()
{
    if (_onSuccess) {
        _onSuccess();
    }
    onSuccess();
}

void GlobalCapabilitiesDirectoryClient::AddOperation::forwardApplicationError(
        const types::DiscoveryError::Enum& e)
{
    if (_onApplicationError) {
        _onApplicationError(e);
    }
    onError(std::make_shared<exceptions::JoynrRuntimeException>(
            "Stop add operation due to ApplicationException."));
}

void GlobalCapabilitiesDirectoryClient::AddOperation::retryOrForwardRuntimeError(
        const exceptions::JoynrRuntimeException& e)
{
    if (!_awaitGlobalRegistration && typeid(exceptions::JoynrTimeOutException) == typeid(e)) {
        execute();
    } else {
        if (_onRuntimeError) {
            _onRuntimeError(e);
        }
        onError(std::make_shared<exceptions::JoynrRuntimeException>(e));
    }
}

} // namespace joynr
