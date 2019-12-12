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
#ifndef TESTS_MOCK_MOCKTESTREQUESTCALLER_H
#define TESTS_MOCK_MOCKTESTREQUESTCALLER_H

#include <gmock/gmock.h>

#include "joynr/tests/testRequestCaller.h"

#include "tests/mock/MockTestProvider.h"

class MockTestRequestCaller : public joynr::tests::testRequestCaller {
public:
    void invokeLocationOnSuccessFct(std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess,
                            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> /*onError*/) {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    void invokeListOfStringsOnSuccessFct(std::function<void(const std::vector<std::string>&)> onSuccess,
                            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> /*onError*/) {
        std::vector<std::string> listOfStrings;
        listOfStrings.push_back("firstString");
        onSuccess(listOfStrings);
    }

    void invokeGetterOnErrorFunctionWithProviderRuntimeException(std::function<void(const std::int32_t&)> /*onSuccess*/,
            std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError) {
        onError(std::make_shared<joynr::exceptions::ProviderRuntimeException>(_providerRuntimeExceptionTestMsg));
    }

    void invokeMethodOnErrorFunctionWithProviderRuntimeException(std::function<void()> /*onSuccess*/,
            std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)> onError) {
        onError(std::make_shared<joynr::exceptions::ProviderRuntimeException>(_providerRuntimeExceptionTestMsg));
    }

    void invokeMapParametersOnSuccessFct(const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
                                         std::function<void(const joynr::types::TestTypes::TStringKeyMap&)> onSuccess,
                                         std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)>) {
        onSuccess(tStringMapIn);
    }

    const joynr::types::Version& getProviderVersion() const {
        return _providerVersion;
    }

    MockTestRequestCaller() :
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>()),
            _providerVersion(47, 11)
    {
        ON_CALL(
                *this,
                getLocationMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeLocationOnSuccessFct));
        ON_CALL(
                *this,
                getListOfStringsMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeListOfStringsOnSuccessFct));
        ON_CALL(
                *this,
                getAttributeWithProviderRuntimeExceptionMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeGetterOnErrorFunctionWithProviderRuntimeException));
        ON_CALL(
                *this,
                methodWithProviderRuntimeExceptionMock(_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeMethodOnErrorFunctionWithProviderRuntimeException));
        ON_CALL(
                *this,
                mapParametersMock(_,_,_)
        )
                .WillByDefault(testing::Invoke(this, &MockTestRequestCaller::invokeMapParametersOnSuccessFct));

    }
    MockTestRequestCaller(testing::Cardinality getLocationCardinality) :
            joynr::tests::testRequestCaller(std::make_shared<MockTestProvider>()),
            _providerVersion(47, 11)
    {
        EXPECT_CALL(
                *this,
                getLocationMock(_,_)
        )
                .Times(getLocationCardinality)
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeLocationOnSuccessFct));
        EXPECT_CALL(
                *this,
                getListOfStringsMock(_,_)
        )
                .WillRepeatedly(testing::Invoke(this, &MockTestRequestCaller::invokeListOfStringsOnSuccessFct));
    }

    // GoogleMock does not support mocking functions with r-value references as parameters
    MOCK_METHOD2(getLocationMock,
                 void(std::function<void(const joynr::types::Localisation::GpsLocation& location)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));

    void getLocation(
                std::function<void(const joynr::types::Localisation::GpsLocation&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getLocationMock(onSuccess, onError);
    }

    MOCK_METHOD3(mapParametersMock,
                 void(const joynr::types::TestTypes::TStringKeyMap&,
                      std::function<void(const joynr::types::TestTypes::TStringKeyMap&)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)>));

    void mapParameters(
                const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
                std::function<void(const joynr::types::TestTypes::TStringKeyMap& tStringMapOut)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)> onError
        ) override
    {
        mapParametersMock(tStringMapIn, onSuccess, onError);
    }

    MOCK_METHOD2(getListOfStringsMock,
                 void(std::function<void(const std::vector<std::string>& listOfStrings)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));

    void getListOfStrings(
                std::function<void(const std::vector<std::string>&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getListOfStringsMock(onSuccess, onError);
    }

    MOCK_METHOD2(getAttributeWithProviderRuntimeExceptionMock,
                 void(std::function<void(const std::int32_t&)>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)>));

    void getAttributeWithProviderRuntimeException(
                std::function<void(const std::int32_t&)>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::ProviderRuntimeException>&)> onError
        ) override
    {
        getAttributeWithProviderRuntimeExceptionMock(onSuccess, onError);
    }

    MOCK_METHOD2(methodWithProviderRuntimeExceptionMock,
                 void(std::function<void()>,
                      std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)>));

    void methodWithProviderRuntimeException(
                std::function<void()>&& onSuccess,
                std::function<void(const std::shared_ptr<joynr::exceptions::JoynrException>&)> onError
        ) override
    {
        methodWithProviderRuntimeExceptionMock(onSuccess, onError);
    }

    MOCK_METHOD2(registerAttributeListener, void(const std::string& attributeName, std::shared_ptr<joynr::SubscriptionAttributeListener> attributeListener));
    MOCK_METHOD2(registerBroadcastListener, void(const std::string& broadcastName, std::shared_ptr<joynr::UnicastBroadcastListener> broadcastListener));
    MOCK_METHOD2(unregisterAttributeListener, void(const std::string& attributeName, std::shared_ptr<joynr::SubscriptionAttributeListener> attributeListener));
    MOCK_METHOD2(unregisterBroadcastListener, void(const std::string& broadcastName, std::shared_ptr<joynr::UnicastBroadcastListener> broadcastListener));

    std::string _providerRuntimeExceptionTestMsg = "ProviderRuntimeExceptionTestMessage";

private:
    joynr::types::Version _providerVersion;
};

#endif // TESTS_MOCK_MOCKTESTREQUESTCALLER_H
