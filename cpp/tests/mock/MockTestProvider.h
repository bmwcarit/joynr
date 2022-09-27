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
#ifndef TESTS_MOCK_MOCKTESTPROVIDER_H
#define TESTS_MOCK_MOCKTESTPROVIDER_H

#include <numeric>
#include <string>
#include <vector>

#include "tests/utils/Gmock.h"

#include "joynr/tests/DefaulttestProvider.h"

using ::testing::_;

class MockTestProvider : public joynr::tests::DefaulttestProvider
{
public:
    MockTestProvider() : joynr::tests::DefaulttestProvider(), _listOfStrings()
    {
        EXPECT_CALL(*this, getLocation(_, _))
                .WillRepeatedly(testing::Invoke(this, &MockTestProvider::invokeLocationOnSuccess));
        EXPECT_CALL(*this, getListOfStrings(_, _))
                .WillRepeatedly(
                        testing::Invoke(this, &MockTestProvider::invokeListOfStringsOnSuccess));
        EXPECT_CALL(*this, getAttributeWithProviderRuntimeException(_, _))
                .WillRepeatedly(testing::Invoke(
                        this,
                        &MockTestProvider::invokeAttributeWithProviderRuntimeExceptionOnError));
    }

    ~MockTestProvider()
    {
        EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(this));
    }

    void invokeLocationOnSuccess(
            std::function<void(const joynr::types::Localisation::GpsLocation&)> onSuccess,
            std::function<
                    void(const joynr::exceptions::ProviderRuntimeException& exception)> /*onError*/)
    {
        joynr::types::Localisation::GpsLocation gpsLocation;
        onSuccess(gpsLocation);
    }

    void invokeListOfStringsOnSuccess(
            std::function<void(const std::vector<std::string>&)> onSuccess,
            std::function<
                    void(const joynr::exceptions::ProviderRuntimeException& exception)> /*onError*/)
    {
        onSuccess(_listOfStrings);
    }

    void invokeAttributeWithProviderRuntimeExceptionOnError(
            std::function<void(const int32_t&)> /*onSuccess*/,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError)
    {
        onError(joynr::exceptions::ProviderRuntimeException(_providerRuntimeExceptionTestMsg));
    }

    void fireLocationUpdateSelective(
            const joynr::types::Localisation::GpsLocation& gpsLocation) override
    {
        joynr::tests::testAbstractProvider::fireLocationUpdateSelective(gpsLocation);
    }

    void fireBroadcastWithSingleArrayParameter(
            const std::vector<std::string>& singleParam,
            const std::vector<std::string>& partitions = std::vector<std::string>()) override
    {
        std::ignore = partitions;
        joynr::tests::testAbstractProvider::fireBroadcastWithSingleArrayParameter(singleParam);
    }

    void listOfStringsChanged(const std::vector<std::string>& listOfStrs) override
    {
        joynr::tests::testAbstractProvider::listOfStringsChanged(listOfStrs);
    }

    void registerBroadcastListener(
            std::shared_ptr<joynr::MulticastBroadcastListener> broadcastListener) override
    {
        joynr::tests::testAbstractProvider::registerBroadcastListener(broadcastListener);
    }

    void methodWithNoInputParameters(
            std::function<void(const std::int32_t& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override
    {
        methodWithNoInputParametersMock([](const std::int32_t&) {},
                                        [](const joynr::exceptions::ProviderRuntimeException&) {});
        joynr::tests::DefaulttestProvider::methodWithNoInputParameters(
                std::move(onSuccess), std::move(onError));
    }

    MOCK_METHOD2(
            methodWithNoInputParametersMock,
            void(std::function<void(const std::int32_t& result)> onSuccess,
                 std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError));

    MOCK_METHOD2(
            getLocation,
            void(std::function<void(const joynr::types::Localisation::GpsLocation& result)>
                         onSuccess,
                 std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                         onError));
    MOCK_METHOD2(
            getListOfStrings,
            void(std::function<void(const std::vector<std::string>& result)> onSuccess,
                 std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                         onError));
    MOCK_METHOD3(
            setLocation,
            void(const joynr::types::Localisation::GpsLocation& gpsLocation,
                 std::function<void()> onSuccess,
                 std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                         onError));
    MOCK_METHOD3(methodFireAndForget,
                 void(const std::int32_t& intIn,
                      const std::string& stringIn,
                      const joynr::tests::testTypes::ComplexTestType& complexTestTypeIn));
    MOCK_METHOD2(
            getAttributeWithProviderRuntimeException,
            void(std::function<void(const int32_t& result)> onSuccess,
                 std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                         onError));

    void sumInts(const std::vector<std::int32_t>& ints,
                 std::function<void(const std::int32_t& result)> onSuccess,
                 std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                            exception)> /*onError*/) override
    {
        std::int32_t result = std::accumulate(ints.begin(), ints.end(), 0);
        onSuccess(result);
    }

    void sumIntsDelayed(const std::vector<std::int32_t>& ints,
                        std::function<void(const std::int32_t& result)> onSuccess,
                        std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                                   exception)> /*onError*/) override
    {
        std::int32_t result = std::accumulate(ints.begin(), ints.end(), 0);
        std::this_thread::sleep_for(std::chrono::seconds(10));
        onSuccess(result);
    }

    void returnPrimeNumbers(const std::int32_t& upperBound,
                            std::function<void(const std::vector<std::int32_t>& result)> onSuccess,
                            std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                                       exception)> /*onError*/) override
    {
        std::vector<std::int32_t> result;
        assert(upperBound < 7);
        std::ignore = upperBound;
        result.clear();
        result.push_back(2);
        result.push_back(3);
        result.push_back(5);
        onSuccess(result);
    }

    void optimizeTrip(const joynr::types::Localisation::Trip& input,
                      std::function<void(const joynr::types::Localisation::Trip& result)> onSuccess,
                      std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                                 exception)> /*onError*/) override
    {
        onSuccess(input);
    }
    void optimizeLocationList(
            const std::vector<joynr::types::Localisation::GpsLocation>& inputList,
            std::function<void(const std::vector<joynr::types::Localisation::GpsLocation>& result)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                       exception)> /*onError*/) override

    {
        onSuccess(inputList);
    }

    void overloadedOperation(const joynr::tests::testTypes::DerivedStruct& /*input*/,
                             std::function<void(const std::string& result)> onSuccess,
                             std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                                        exception)> /*onError*/) override
    {
        std::string result("DerivedStruct");
        onSuccess(result);
    }

    void overloadedOperation(const joynr::tests::testTypes::AnotherDerivedStruct& /*input*/,
                             std::function<void(const std::string& result)> onSuccess,
                             std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                                        exception)> /*onError*/) override
    {
        std::string result("AnotherDerivedStruct");
        onSuccess(result);
    }

    void setListOfStrings(const std::vector<std::string>& listOfStrs,
                          std::function<void()> onSuccess,
                          std::function<void(const joynr::exceptions::ProviderRuntimeException&
                                                     exception)> /*onError*/
                          ) override
    {
        this->_listOfStrings = listOfStrs;
        listOfStringsChanged(listOfStrs);
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
            std::function<void(const bool& booleanOut,
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
                               const std::uint8_t& uInt8Out)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override
    {
        std::ignore = onError;
        onSuccess(booleanArg, doubleArg, floatArg, int16Arg, int32Arg, int64Arg, int8Arg, stringArg,
                  uInt16Arg, uInt32Arg, uInt64Arg, uInt8Arg);
    }

    void methodWithByteBuffer(
            const joynr::ByteBuffer& input,
            std::function<void(const joynr::ByteBuffer& result)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override
    {
        std::ignore = onError;
        onSuccess(input);
    }

    void mapParameters(
            const joynr::types::TestTypes::TStringKeyMap& tStringMapIn,
            std::function<void(const joynr::types::TestTypes::TStringKeyMap& tStringMapOut)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override
    {
        std::ignore = onError;
        onSuccess(tStringMapIn);
    }
    const std::string _providerRuntimeExceptionTestMsg = "ProviderRuntimeExceptionTestMessage";

private:
    std::vector<std::string> _listOfStrings;
};

#endif // TESTS_MOCK_MOCKTESTPROVIDER_H
