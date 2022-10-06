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
#ifndef MOCKCALLBACK_H_
#define MOCKCALLBACK_H_

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "joynr/exceptions/JoynrException.h"

#include "tests/PrettyPrint.h"

template <typename... Ts>
class MockCallback
{
public:
    MOCK_METHOD1_T(onFatalRuntimeError, void(const joynr::exceptions::JoynrException& error));
    MOCK_METHOD1_T(onSuccess, void(const Ts&... result));
    MOCK_METHOD1_T(onError, void(const joynr::exceptions::JoynrException& error));
};

template <typename T>
class MockCallback<T&&>
{
public:
    MOCK_METHOD1_T(onFatalRuntimeError, void(const joynr::exceptions::JoynrException& error));
    MOCK_METHOD1_T(onSuccess, void(const T& result));
    MOCK_METHOD1_T(onError, void(const joynr::exceptions::JoynrException& error));
    void onSuccess(T&& result)
    {
        this->onSuccess(result);
    }
};

template <typename T1, typename T2>
class MockCallback2
{
public:
    MOCK_METHOD1_T(onFatalRuntimeError, void(const joynr::exceptions::JoynrException& error));
    MOCK_METHOD2_T(onSuccess, void(const T1& out1, const T2& out2));
    MOCK_METHOD1_T(onError, void(const joynr::exceptions::JoynrException& error));
};

template <>
class MockCallback<void>
{
public:
    MOCK_METHOD1(onFatalRuntimeError, void(const joynr::exceptions::JoynrException& error));
    MOCK_METHOD0(onSuccess, void(void));
    MOCK_METHOD1(onError, void(const joynr::exceptions::JoynrException& error));
};

template <typename T, typename ErrorEnum>
class MockCallbackWithApplicationError
{
public:
    MOCK_METHOD1_T(onSuccess, void(const T& result));
    MOCK_METHOD1_T(onApplicationError, void(const ErrorEnum& errorEnum));
    MOCK_METHOD1_T(onRuntimeError,
                   void(const joynr::exceptions::JoynrRuntimeException& runtimeError));
};

template <typename ErrorEnum>
class MockCallbackWithApplicationError<void, ErrorEnum>
{
public:
    MOCK_METHOD0_T(onSuccess, void(void));
    MOCK_METHOD1_T(onApplicationError, void(const ErrorEnum& errorEnum));
    MOCK_METHOD1_T(onRuntimeError,
                   void(const joynr::exceptions::JoynrRuntimeException& runtimeError));
};

template <typename... Ts>
class MockCallbackWithJoynrException
{
public:
    MOCK_METHOD1_T(onSuccess, void(const Ts&... result));
    MOCK_METHOD1_T(onError, void(const std::shared_ptr<joynr::exceptions::JoynrException>& error));
};

template <>
class MockCallbackWithJoynrException<void>
{

public:
    MOCK_METHOD0(onSuccess, void(void));
    MOCK_METHOD1(onError, void(const std::shared_ptr<joynr::exceptions::JoynrException>& error));
};

#endif // MOCKCALLBACK_H_
