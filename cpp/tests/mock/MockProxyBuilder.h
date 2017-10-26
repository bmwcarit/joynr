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
#ifndef TESTS_MOCK_MOCKPROXYBUILDER_H
#define TESTS_MOCK_MOCKPROXYBUILDER_H

#include <gmock/gmock.h>

#include "joynr/IProxyBuilder.h"

template<typename T>
class MockProxyBuilder : public joynr::IProxyBuilder<T>
{
public:
    MockProxyBuilder(){
        using ::testing::_;
        using ::testing::Return;
        ON_CALL(*this, setCached(_)).WillByDefault(Return(this));
        ON_CALL(*this, setMessagingQos(_)).WillByDefault(Return(this));
        ON_CALL(*this, setDiscoveryQos(_)).WillByDefault(Return(this));
    }

    MOCK_METHOD1_T(setCached, joynr::IProxyBuilder<T>*(const bool cached));
    MOCK_METHOD1_T(setMessagingQos, joynr::IProxyBuilder<T>*(const joynr::MessagingQos& cached));
    MOCK_METHOD1_T(setDiscoveryQos, joynr::IProxyBuilder<T>*(const joynr::DiscoveryQos& cached));
    MOCK_METHOD0_T(build, std::shared_ptr<T>());
    MOCK_METHOD2_T(buildAsync, void(std::function<void(std::shared_ptr<T> proxy)> onSuccess,
                                    std::function<void(const joynr::exceptions::DiscoveryException&)>));
};

#endif // TESTS_MOCK_MOCKPROXYBUILDER_H
