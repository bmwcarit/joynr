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
#ifndef TESTS_MOCK_MOCKGLOBALDOMAINROLECONTROLLERPROXY_H
#define TESTS_MOCK_MOCKGLOBALDOMAINROLECONTROLLERPROXY_H

#include <gmock/gmock.h>

#include "joynr/infrastructure/GlobalDomainRoleControllerProxy.h"

namespace joynr
{
    class JoynrRuntimeImpl;
} // namespace joynr

class MockGlobalDomainRoleControllerProxy : public virtual joynr::infrastructure::GlobalDomainRoleControllerProxy {
public:
    MockGlobalDomainRoleControllerProxy(std::weak_ptr<joynr::JoynrRuntimeImpl> runtime) :
        ProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainRoleControllerProxyBase(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainRoleControllerSyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainRoleControllerAsyncProxy(
                runtime,
                nullptr,
                "domain",
                joynr::MessagingQos()),
        GlobalDomainRoleControllerProxy(
                runtime,
                nullptr,
                  "domain",
                joynr::MessagingQos())
    {
    }

    std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>>>
            getDomainRolesAsync(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>& domainRoleEntries
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            ) noexcept override
    {
        return getDomainRolesAsyncMock(uid, std::move(onSuccess), std::move(onError), std::move(qos));
    }
    MOCK_METHOD4(
            getDomainRolesAsyncMock,
            std::shared_ptr<joynr::Future<std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>>>(
                const std::string& uid,
                std::function<void(
                    const std::vector<joynr::infrastructure::DacTypes::DomainRoleEntry>& domainRoleEntries
                )> onSuccess,
                std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onError,
                boost::optional<joynr::MessagingQos> qos
            )
    );
};

#endif // TESTS_MOCK_MOCKGLOBALDOMAINROLECONTROLLERPROXY_H
