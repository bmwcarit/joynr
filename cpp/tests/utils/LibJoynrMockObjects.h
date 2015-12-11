/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#ifndef LIBJOYNR_MOCKOBJECTS_H_
#define LIBJOYNR_MOCKOBJECTS_H_

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include "PrettyPrint.h"

#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/ISubscriptionListener.h"

using ::testing::A;
using ::testing::_;
using ::testing::A;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Property;

// Disable VC++ warnings due to google mock
// http://code.google.com/p/googlemock/wiki/FrequentlyAskedQuestions#MSVC_gives_me_warning_C4301_or_C4373_when_I_define_a_mock_method
#ifdef _MSC_VER
    #pragma warning( push )
    #pragma warning( disable : 4373 )
#endif

// Disable compiler warnings.
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wreorder"

// GMock doesn't support mocking variadic template functions directly.
// Workaround: Mock exactly the functions with the number of arguments used in the tests.
template <typename T>
class MockSubscriptionListenerOneType : public joynr::ISubscriptionListener<T> {
public:
     MOCK_METHOD1_T(onReceive, void( const T& value));
     MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

template <typename T1, typename T2, typename... Ts>
class MockSubscriptionListenerTwoTypes : public joynr::ISubscriptionListener<T1, T2, Ts...> {
public:
     MOCK_METHOD2_T(onReceive, void( const T1& value1, const T2& value2, const Ts&... values));
     MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

class MockGpsSubscriptionListener : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation> {
public:
    MOCK_METHOD1(onReceive, void(const joynr::types::Localisation::GpsLocation& value));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

class MockTestProvider : public joynr::tests::DefaulttestProvider
{
public:
    MockTestProvider() :
        listOfStrings(),
        joynr::tests::DefaulttestProvider()
    {
        EXPECT_CALL(*this, getLocation(_,_))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeLocationOnSuccess));
        EXPECT_CALL(*this, getListOfStrings(_,_))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeListOfStringsOnSuccess));
    }
    MockTestProvider(joynr::types::ProviderQos qos) :
        DefaulttestProvider()
    {
        providerQos = qos;
        EXPECT_CALL(*this, getLocation(_,_))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeLocationOnSuccess));
        EXPECT_CALL(*this, getListOfStrings(_,_))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeListOfStringsOnSuccess));
    }
    ~MockTestProvider()
    {
    };

    void invokeLocationOnSuccess(std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess,
                         std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    void invokeListOfStringsOnSuccess(std::function<void(const std::vector<std::string>&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) {
        onSuccess(listOfStrings);
    }

    void fireLocationUpdateSelective(const joynr::types::Localisation::GpsLocation& location) {
        joynr::tests::testAbstractProvider::fireLocationUpdateSelective(location);
    }

    void fireBroadcastWithSingleArrayParameter(const std::vector<std::string> singleParam) {
        joynr::tests::testAbstractProvider::fireBroadcastWithSingleArrayParameter(singleParam);
    }

    void listOfStringsChanged(const std::vector<std::string> listOfStrings) {
        joynr::tests::testAbstractProvider::listOfStringsChanged(listOfStrings);
    }

    MOCK_METHOD2(
            getLocation,
            void(
                    std::function<void(const joynr::types::Localisation::GpsLocation& result)> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
            )
    );
    MOCK_METHOD2(
            getListOfStrings,
            void(
                    std::function<void(const std::vector<std::string>& result)> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
            )
    );
    MOCK_METHOD3(
            setLocation,
            void(
                    const joynr::types::Localisation::GpsLocation& gpsLocation,
                    std::function<void()> onSuccess,
                    std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
            )
    );

    void sumInts(
            const std::vector<int32_t>& ints,
            std::function<void(const int32_t& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
        int32_t result = 0;
        int32_t j;
        foreach ( j, ints) {
            result += j;
        }
        onSuccess(result);
    }
    void returnPrimeNumbers(
            const int32_t &upperBound,
            std::function<void(
                const std::vector<int32_t>& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
        std::vector<int32_t> result;
        assert(upperBound<7);
        result.clear();
        result.push_back(2);
        result.push_back(3);
        result.push_back(5);
        onSuccess(result);
    }
    void optimizeTrip(
            const joynr::types::Localisation::Trip& input,
            std::function<void(
                const joynr::types::Localisation::Trip& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
         onSuccess(input);
    }
    void optimizeLocationList(
            const std::vector<joynr::types::Localisation::GpsLocation>& inputList,
            std::function<void(
                const std::vector<joynr::types::Localisation::GpsLocation>& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)

    {
         onSuccess(inputList);
    }

    void overloadedOperation(
            const joynr::tests::testTypes::DerivedStruct& input,
            std::function<void(
                const std::string& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
        std::string result("QtDerivedStruct");
        onSuccess(result);
    }

    void overloadedOperation(
            const joynr::tests::testTypes::AnotherDerivedStruct& input,
            std::function<void(
                const std::string& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
        std::string result("QtAnotherDerivedStruct");
        onSuccess(result);
    }

    void setListOfStrings(
         const std::vector<std::string> & listOfStrings,
         std::function<void()> onSuccess,
         std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
    ) {
        this->listOfStrings = listOfStrings;
        listOfStringsChanged(listOfStrings);
        onSuccess();
    }

    void mapParameters(
            const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
            std::function<void(
                    const joynr::types::TestTypes::TStringKeyMap& tStringMapOut
            )> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    ) {
        (void) onError;
        onSuccess(
                tStringMapIn
        );
    }
    std::string providerRuntimeExceptionTestMsg = "ProviderRuntimeExceptionTestMessage";

private:
    std::vector<std::string> listOfStrings;
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif /* LIBJOYNR_MOCKOBJECTS_H_ */
