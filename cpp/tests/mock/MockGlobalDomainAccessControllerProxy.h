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

class MockGlobalDomainAccessControllerProxy : public virtual joynr::infrastructure::GlobalDomainAccessControllerProxy {
public:
    MockGlobalDomainAccessControllerProxy(std::weak_ptr<joynr::JoynrRuntime> runtime) :
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

    MOCK_METHOD4(
            getMasterAccessControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD4(
            getMediatorAccessControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterAccessControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD4(
            getOwnerAccessControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>>>(
                const std::string& domain,
                const std::string& interfaceName,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerAccessControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
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

    MOCK_METHOD3(
            getMasterRegistrationControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>& masterAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD3(
            getMediatorRegistrationControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::MasterRegistrationControlEntry>& mediatorAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
            )
    );

    MOCK_METHOD3(
            getOwnerRegistrationControlEntriesAsync,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry>& ownerAces
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError
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
