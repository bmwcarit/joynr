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

#include "GlobalCapabilitiesDirectoryClient.h"

#include "joynr/ClusterControllerSettings.h"
#include "joynr/Message.h"
#include "joynr/Semaphore.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/types/GlobalDiscoveryEntry.h"
#include "joynr/TimePoint.h"

namespace joynr
{

GlobalCapabilitiesDirectoryClient::GlobalCapabilitiesDirectoryClient(
        const ClusterControllerSettings& clusterControllerSettings)
        : _capabilitiesProxy(nullptr),
          _messagingQos(),
          _touchTtl(static_cast<std::uint64_t>(
                  clusterControllerSettings.getCapabilitiesFreshnessUpdateIntervalMs().count())),
          _removeStaleTtl(3600000)
{
}

GlobalCapabilitiesDirectoryClient::~GlobalCapabilitiesDirectoryClient()
{
    shutdown();
}

void GlobalCapabilitiesDirectoryClient::shutdown()
{
    // Assure that all captures to class members are released.
    _sequentialTasks.cancel();
}

void GlobalCapabilitiesDirectoryClient::add(
        const types::GlobalDiscoveryEntry& entry,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos addMessagingQos = _messagingQos;
    addMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    using std::move;
    TaskSequencer<void>::TaskWithExpiryDate timeoutTask;
    timeoutTask.expiryDateMs = static_cast<std::uint64_t>(TimePoint::now().toMilliseconds()) +
                               addMessagingQos.getTtl();
    timeoutTask.timeout = []() {};
    timeoutTask.task = [
        this,
        entry,
        gbids,
        onSuccess{move(onSuccess)},
        onError{move(onError)},
        onRuntimeError{move(onRuntimeError)},
        addMessagingQos{move(addMessagingQos)}
    ]() mutable
    {
        auto future = std::make_shared<Future<void>>();
        onSuccess = [ future, onSuccess{move(onSuccess)} ]()
        {
            if (onSuccess) {
                onSuccess();
            }
            future->onSuccess();
        };
        onError = [ future, onError{move(onError)} ](const joynr::types::DiscoveryError::Enum& e)
        {
            if (onError) {
                onError(e);
            }
            future->onError(std::make_shared<exceptions::JoynrRuntimeException>(
                    "Stop add operation due to ApplicationException."));
        };
        onRuntimeError = [ future, onRuntimeError{move(onRuntimeError)} ](
                const exceptions::JoynrRuntimeException& e)
        {
            if (onRuntimeError) {
                onRuntimeError(e);
            }
            future->onError(std::make_shared<exceptions::JoynrRuntimeException>(e));
        };
        _capabilitiesProxy->addAsync(entry,
                                     move(gbids),
                                     move(onSuccess),
                                     move(onError),
                                     move(onRuntimeError),
                                     addMessagingQos);
        return future;
    };

    _sequentialTasks.add(timeoutTask);
}

void GlobalCapabilitiesDirectoryClient::remove(
        const std::string& participantId,
        const std::vector<std::string>& gbids,
        std::function<void()> onSuccess,
        std::function<void(const joynr::types::DiscoveryError::Enum& errorEnum)> onError,
        std::function<void(const exceptions::JoynrRuntimeException& error)> onRuntimeError)
{
    MessagingQos removeMessagingQos = _messagingQos;
    removeMessagingQos.putCustomMessageHeader(Message::CUSTOM_HEADER_GBID_KEY(), gbids[0]);
    auto retryRemoveOperation =
            std::make_shared<RetryRemoveOperation>(_capabilitiesProxy,
                                                   participantId,
                                                   gbids,
                                                   std::move(onSuccess),
                                                   std::move(onError),
                                                   std::move(onRuntimeError),
                                                   std::move(removeMessagingQos));
    TaskSequencer<void>::TaskWithExpiryDate timeoutTask;
    timeoutTask.task = [retryRemoveOperation]() {
        retryRemoveOperation->execute();
        return retryRemoveOperation;
    };
    timeoutTask.expiryDateMs = std::numeric_limits<std::uint64_t>::max();
    timeoutTask.timeout = []() {};

    _sequentialTasks.add(timeoutTask);
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
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy,
        MessagingQos messagingQos)
{
    this->_capabilitiesProxy = std::move(capabilitiesProxy);
    this->_messagingQos = std::move(messagingQos);
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
        const std::vector<std::string>& gbids,
        std::function<void()>&& onSuccessFunc,
        std::function<void(const types::DiscoveryError::Enum&)>&& onApplicationErrorFunc,
        std::function<void(const exceptions::JoynrRuntimeException&)>&& onRuntimeErrorFunc,
        boost::optional<MessagingQos> qos)
        : Future<void>(),
          _capabilitiesProxy{capabilitiesProxy},
          _participantId{participantId},
          _gbids{gbids},
          _onSuccess{onSuccessFunc},
          _onApplicationError{onApplicationErrorFunc},
          _onRuntimeError{onRuntimeErrorFunc},
          _qos{qos}
{
}

void GlobalCapabilitiesDirectoryClient::RetryRemoveOperation::execute()
{
    if (StatusCodeEnum::IN_PROGRESS == getStatus()) {
        std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> capabilitiesProxy =
                _capabilitiesProxy.lock();
        if (capabilitiesProxy) {
            using std::placeholders::_1;
            capabilitiesProxy->removeAsync(
                    _participantId,
                    _gbids,
                    std::bind(&RetryRemoveOperation::forwardSuccess, shared_from_this()),
                    std::bind(
                            &RetryRemoveOperation::forwardApplicationError, shared_from_this(), _1),
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

} // namespace joynr
