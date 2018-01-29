/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

#include "ProviderCheck.h"

#include <chrono>

ProviderCheck::ProviderCheck(int tdelay) : thread_delay_time (tdelay)
{
    exit_ready = false;
    demoAppAttribString1_Current = "";

    thread_1 = std::thread(&ProviderCheck::thread_func_1, this);
    thread_2 = std::thread(&ProviderCheck::thread_func_2, this);
}

ProviderCheck::~ProviderCheck()
{
    JOYNR_LOG_INFO(logger(), "ProviderCheck Destructor called");
    exit_ready=true;
    thread_1.join();
    thread_2.join();
}

void ProviderCheck::fireBroadcast1Msg(
        const std::string& stringArg)
{
    subContainer.setName(stringArg);
    fireBroadcastWithClassParameter1(subContainer);
}

void ProviderCheck::fireBroadcast2Msg(
        const std::string& stringArg)
{
    subContainer.setName(stringArg);
    fireBroadcastWithClassParameter2(subContainer);
}

void ProviderCheck::fireBroadcastSelective1Msg(
        const std::string& stringArg)
{
    fireBroadcastSelectiveWithSingleStringParameter2(stringArg);
}

void ProviderCheck::fireBroadcastSelective2Msg(
        const std::string& stringArg)
{
    fireBroadcastSelectiveWithSingleStringParameter2(stringArg);
}

void ProviderCheck::getAttributeString1(
        std::function<void(const std::string&)> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
    std::lock_guard<std::mutex> lock_method1(mutex_method1);
    std::ignore = onError;
    onSuccess(attributeString1);
}

void ProviderCheck::setAttributeString1(
        const std::string& attributeString1,
        std::function<void()> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
    std::lock_guard<std::mutex> lock_method1(mutex_method1);
    std::ignore = onError;
    this->attributeString1 = attributeString1;
    attributeString1Changed(attributeString1);
    onSuccess();
}

void ProviderCheck::getAttributeString2(
        std::function<void(const std::string&)> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
    std::lock_guard<std::mutex> lock_method2(mutex_method2);
    std::ignore = onError;
    onSuccess(attributeString2);
}

void ProviderCheck::setAttributeString2(
        const std::string& attributeString2,
        std::function<void()> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
) {
    std::lock_guard<std::mutex> lock_method2(mutex_method2);
    std::ignore = onError;
    this->attributeString2 = attributeString2;
    attributeString2Changed(attributeString2);
    onSuccess();
}

void ProviderCheck::methodWithStringParameters1(
        const std::string& stringIn,
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    setAttributeString1(stringIn, [&](){}, onError);
    onSuccess("Method 1 : onSuccess");
}

void ProviderCheck::methodWithStringParameters2(
        const std::string& stringIn,
        std::function<void(const std::string& stringOut)> onSuccess,
        std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    setAttributeString2(stringIn, [&](){}, onError);
    onSuccess("Method 2 : onSuccess");
}

void ProviderCheck::thread_func_1()
{
    std::string attribute;

    std::function<void(const std::string&)> onSuccessWrapper =
            [&](const std::string& stringOut)
    {
        attribute = stringOut;
    };

    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
            [&](const joynr::exceptions::ProviderRuntimeException& exception)
    {
        JOYNR_LOG_INFO(logger(), "Exception: " + exception.getMessage());
    };

    // thread loop
    while(!exit_ready)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(thread_delay_time));

        // check for changes in the attribute before broadcasting
        // ToDo: use the existing attribute change notification mechanism instead
        getAttributeString1(onSuccessWrapper, onErrorWrapper);
        if( demoAppAttribString1_Current.compare(attribute) != 0 )
        {
            demoAppAttribString1_Current = attribute;
            fireBroadcast1Msg(demoAppAttribString1_Current);
        }
    }
}

void ProviderCheck::thread_func_2()
{
    std::string attribute;

    std::function<void(const std::string&)> onSuccessWrapper =
            [&](const std::string& stringOut)
    {
        attribute = stringOut;
    };

    std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onErrorWrapper =
            [&](const joynr::exceptions::ProviderRuntimeException& exception)
    {
        JOYNR_LOG_INFO(logger(), "Exception: " + exception.getMessage());
    };

    // thread loop
    while(!exit_ready)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(thread_delay_time));

        // check for changes in the attribute before broadcasting
        // ToDo: use the existing attribute change notification mechanism instead
        getAttributeString2(onSuccessWrapper, onErrorWrapper);
        if( demoAppAttribString2_Current.compare(attribute) != 0 )
        {
            demoAppAttribString2_Current = attribute;
            fireBroadcast2Msg(demoAppAttribString2_Current);
        }
    }
}
