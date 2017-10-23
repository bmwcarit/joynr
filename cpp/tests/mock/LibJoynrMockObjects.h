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
using ::testing::Mock;
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

#ifdef _MSC_VER
    #pragma warning( push )
#endif

// restore GCC diagnostic state
#pragma GCC diagnostic pop

#endif /* LIBJOYNR_MOCKOBJECTS_H_ */
