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
#ifndef LIBJOYNR_MOCKOBJECTS_H_
#define LIBJOYNR_MOCKOBJECTS_H_

#include <memory>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/types/Localisation/GpsLocation.h"
#include "joynr/tests/DefaulttestProvider.h"
#include "joynr/ISubscriptionListener.h"

#include "tests/PrettyPrint.h"

using ::testing::A;
using ::testing::_;
using ::testing::Eq;
using ::testing::NotNull;
using ::testing::AllOf;
using ::testing::Invoke;
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
     MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
     MOCK_METHOD1_T(onReceive, void( const T& value));
     MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

template <typename T1, typename T2>
class MockSubscriptionListenerTwoTypes : public joynr::ISubscriptionListener<T1, T2> {
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD2_T(onReceive, void( const T1& value1, const T2& value2));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

template <typename T1, typename T2, typename T3, typename T4, typename T5>
class MockSubscriptionListenerFiveTypes : public joynr::ISubscriptionListener<T1, T2, T3, T4, T5> {
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD5_T(onReceive, void( const T1& value1, const T2& value2, const T3& value3, const T4& value4, const T5& value5));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

class MockGpsSubscriptionListener : public joynr::ISubscriptionListener<joynr::types::Localisation::GpsLocation> {
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD1(onReceive, void(const joynr::types::Localisation::GpsLocation& value));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrRuntimeException&));
};

class MockSubscriptionListenerZeroTypes : public joynr::ISubscriptionListener<void> {
public:
    MOCK_METHOD1(onSubscribed, void(const std::string& subscriptionId));
    MOCK_METHOD0(onReceive, void());
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

    ~MockTestProvider() = default;

    void invokeLocationOnSuccess(std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess,
                         std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
        joynr::types::Localisation::GpsLocation location;
        onSuccess(location);
    }

    void invokeListOfStringsOnSuccess(std::function<void(const std::vector<std::string>&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError)
    {
        onSuccess(listOfStrings);
    }

    void fireLocationUpdateSelective(const joynr::types::Localisation::GpsLocation& location) override
    {
        joynr::tests::testAbstractProvider::fireLocationUpdateSelective(location);
    }

    void fireBroadcastWithSingleArrayParameter(
            const std::vector<std::string>& singleParam,
            const std::vector<std::string>& partitions = std::vector<std::string>()
    ) override
    {
        joynr::tests::testAbstractProvider::fireBroadcastWithSingleArrayParameter(singleParam);
    }

    void listOfStringsChanged(const std::vector<std::string>& listOfStrings) override
    {
        joynr::tests::testAbstractProvider::listOfStringsChanged(listOfStrings);
    }

    void registerBroadcastListener(std::shared_ptr<joynr::MulticastBroadcastListener> broadcastListener) override
    {
        joynr::tests::testAbstractProvider::registerBroadcastListener(broadcastListener);
    }

    void methodWithNoInputParameters(std::function<void(const std::int32_t& result)> onSuccess,
                                     std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError) override
    {
        methodWithNoInputParametersMock([](const std::int32_t&){},
                                        [](const joynr::exceptions::ProviderRuntimeException&){});
        joynr::tests::DefaulttestProvider::methodWithNoInputParameters(std::move(onSuccess), std::move(onError));
    }

    MOCK_METHOD2(
            methodWithNoInputParametersMock,
            void(std::function<void(
                    const std::int32_t& result
                 )> onSuccess,
                 std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError)
    );

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
    MOCK_METHOD3(
            methodFireAndForget,
            void(
                    const std::int32_t& intIn,
                    const std::string& stringIn,
                    const joynr::tests::testTypes::ComplexTestType& complexTestTypeIn
            )
    );

    void sumInts(
            const std::vector<std::int32_t>& ints,
            std::function<void(const std::int32_t& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) override
    {
        std::int32_t result = std::accumulate(ints.begin(), ints.end(), 0);
        onSuccess(result);
    }

    void returnPrimeNumbers(
            const std::int32_t &upperBound,
            std::function<void(
                const std::vector<std::int32_t>& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) override
    {
        std::vector<std::int32_t> result;
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
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) override
    {
         onSuccess(input);
    }
    void optimizeLocationList(
            const std::vector<joynr::types::Localisation::GpsLocation>& inputList,
            std::function<void(
                const std::vector<joynr::types::Localisation::GpsLocation>& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) override

    {
         onSuccess(inputList);
    }

    void overloadedOperation(
            const joynr::tests::testTypes::DerivedStruct& input,
            std::function<void(
                const std::string& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) override
    {
        std::string result("DerivedStruct");
        onSuccess(result);
    }

    void overloadedOperation(
            const joynr::tests::testTypes::AnotherDerivedStruct& input,
            std::function<void(
                const std::string& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError) override
    {
        std::string result("AnotherDerivedStruct");
        onSuccess(result);
    }

    void setListOfStrings(
         const std::vector<std::string> & listOfStrings,
         std::function<void()> onSuccess,
         std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)> onError
    ) override
    {
        this->listOfStrings = listOfStrings;
        listOfStringsChanged(listOfStrings);
        onSuccess();
    }

    void methodWithAllPossiblePrimitiveParameters(
            const bool& booleanArg,
            const double& doubleArg,
            const float& floatArg,
            const std::int16_t& int16Arg,
            const std::int32_t& int32Arg,
            const std::int64_t& int64Arg,
            const std::int8_t& int8Arg,
            const std::string& stringArg,
            const std::uint16_t& uInt16Arg,
            const std::uint32_t& uInt32Arg,
            const std::uint64_t& uInt64Arg,
            const std::uint8_t& uInt8Arg,
            std::function<void(
                    const bool& booleanOut,
                    const double& doubleOut,
                    const float& floatOut,
                    const std::int16_t& int16Out,
                    const std::int32_t& int32Out,
                    const std::int64_t& int64Out,
                    const std::int8_t& int8Out,
                    const std::string& stringOut,
                    const std::uint16_t& uInt16Out,
                    const std::uint32_t& uInt32Out,
                    const std::uint64_t& uInt64Out,
                    const std::uint8_t& uInt8Out
            )> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    ) override
    {
        std::ignore = onError;
        onSuccess(
                booleanArg, doubleArg, floatArg, int16Arg, int32Arg, int64Arg, int8Arg, stringArg, uInt16Arg, uInt32Arg, uInt64Arg, uInt8Arg
        );
    }

    void methodWithByteBuffer(
            const joynr::ByteBuffer& input,
            std::function<void(
                    const joynr::ByteBuffer& result
            )> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    ) override
    {
        std::ignore = onError;
        onSuccess(
                input
        );
    }

    void mapParameters(
            const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
            std::function<void(
                    const joynr::types::TestTypes::TStringKeyMap& tStringMapOut
            )> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    ) override
    {
        std::ignore = onError;
        onSuccess(
                tStringMapIn
        );
    }

private:
    std::vector<std::string> listOfStrings;
};

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif /* LIBJOYNR_MOCKOBJECTS_H_ */
