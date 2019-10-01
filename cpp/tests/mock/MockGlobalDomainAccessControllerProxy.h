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
#ifndef TESTS_MOCK_MOCKGLOBALDOMAINACCESSCONTROLLERPROXY_H
#define TESTS_MOCK_MOCKGLOBALDOMAINACCESSCONTROLLERPROXY_H

#include <gmock/gmock.h>

#include "joynr/infrastructure/GlobalDomainAccessControllerProxy.h"

namespace joynr
{
class JoynrRuntimeImpl;
} // namespace joynr

class MockGlobalDomainAccessControllerProxy : public virtual joynr::infrastructure::GlobalDomainAccessControllerProxy {
public:
    MockGlobalDomainAccessControllerProxy(std::weak_ptr<joynr::JoynrRuntimeImpl> runtime) :
        ProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerSyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerAsyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainAccessControllerProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos())
    {
    }

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>
            getMasterAccessControlEntriesAsync(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getMasterAccessControlEntriesAsyncMock(domain, interfaceName, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD5(
            getMasterAccessControlEntriesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                const std::string& _domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>
            getMediatorAccessControlEntriesAsync(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getMediatorAccessControlEntriesAsyncMock(domain, interfaceName, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD5(
            getMediatorAccessControlEntriesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                const std::string& _domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>>>
            getOwnerAccessControlEntriesAsync(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getOwnerAccessControlEntriesAsyncMock(domain, interfaceName, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD5(
            getOwnerAccessControlEntriesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>>>(
                const std::string& _domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD3(
            subscribeToMasterAccessControlEntryChangedBroadcast,
            std::shared_ptr<joynr::Future<std::string>>(
                std::shared_ptr<
                    joynr::ISubscriptionListener<
                        joynr::infrastructure::DacTypes::ChangeType::Enum,
                        joynr::infrastructure::DacTypes::MasterAccessControlEntry>
                    >subscriptionListener,
                std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
                const std::vector<std::string>& partitions
            )
    );

    MOCK_METHOD3(
            subscribeToMediatorAccessControlEntryChangedBroadcast,
            std::shared_ptr<joynr::Future<std::string>>(
                std::shared_ptr<
                    joynr::ISubscriptionListener<
                        joynr::infrastructure::DacTypes::ChangeType::Enum,
                        joynr::infrastructure::DacTypes::MasterAccessControlEntry>
                    >subscriptionListener,
                std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
                const std::vector<std::string>& partitions
            )
    );

    MOCK_METHOD3(
            subscribeToOwnerAccessControlEntryChangedBroadcast,
            std::shared_ptr<joynr::Future<std::string>>(
                std::shared_ptr<
                    joynr::ISubscriptionListener<
                        joynr::infrastructure::DacTypes::ChangeType::Enum,
                        joynr::infrastructure::DacTypes::OwnerAccessControlEntry>
                    >subscriptionListener,
                std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
                const std::vector<std::string>& partitions
            )
    );

    // Registration control

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>>>
            getMasterRegistrationControlEntriesAsync(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getMasterRegistrationControlEntriesAsyncMock(uid, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD4(
            getMasterRegistrationControlEntriesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>>>
            getMediatorRegistrationControlEntriesAsync(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getMediatorRegistrationControlEntriesAsyncMock(uid, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD4(
            getMediatorRegistrationControlEntriesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>>>
            getOwnerRegistrationControlEntriesAsync(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getOwnerRegistrationControlEntriesAsyncMock(uid, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD4(
            getOwnerRegistrationControlEntriesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );

    MOCK_METHOD3(
            subscribeToMasterRegistrationControlEntryChangedBroadcast,
            std::shared_ptr<joynr::Future<std::string>>(
                std::shared_ptr<
                    joynr::ISubscriptionListener<
                        joynr::infrastructure::DacTypes::ChangeType::Enum,
                        joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>
                    >subscriptionListener,
                std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
                const std::vector<std::string>& partitions
            )
    );

    MOCK_METHOD3(
            subscribeToMediatorRegistrationControlEntryChangedBroadcast,
            std::shared_ptr<joynr::Future<std::string>>(
                std::shared_ptr<
                    joynr::ISubscriptionListener<
                        joynr::infrastructure::DacTypes::ChangeType::Enum,
                        joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>
                    >subscriptionListener,
                std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
                const std::vector<std::string>& partitions
            )
    );

    MOCK_METHOD3(
            subscribeToOwnerRegistrationControlEntryChangedBroadcast,
            std::shared_ptr<joynr::Future<std::string>>(
                std::shared_ptr<
                    joynr::ISubscriptionListener<
                        joynr::infrastructure::DacTypes::ChangeType::Enum,
                        joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>
                    >subscriptionListener,
                std::shared_ptr<joynr::MulticastSubscriptionQos> subscriptionQos,
                const std::vector<std::string>& partitions
            )
    );
};

#endif // TESTS_MOCK_MOCKGLOBALDOMAINACCESSCONTROLLERPROXY_H
