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

#ifndef PROVIDERCHECK_H
#define PROVIDERCHECK_H

#include <thread>
#include <mutex>

#include "joynr/tests/robustness/DefaultTestInterfaceProvider.h"
#include "joynr/Logger.h"

class ProviderCheck: public joynr::tests::robustness::DefaultTestInterfaceProvider
{
public:
    ProviderCheck(int tdelay);
    ~ProviderCheck();

    void fireBroadcast1Msg(
            const std::string& stringArg);

    void fireBroadcast2Msg(
            const std::string& stringArg);

    void fireBroadcastSelective1Msg(
            const std::string& stringArg);

    void fireBroadcastSelective2Msg(
            const std::string& stringArg);

    void getAttributeString1(
            std::function<void(const std::string&)> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

    void setAttributeString1(
            const std::string& attributeString1,
            std::function<void()> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

    void getAttributeString2(
            std::function<void(const std::string&)> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

    void setAttributeString2(
            const std::string& attributeString2,
            std::function<void()> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

    void methodWithStringParameters1(const std::string& stringIn,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

    void methodWithStringParameters2(const std::string& stringIn,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void (const joynr::exceptions::ProviderRuntimeException&)> onError
    );

private:
    ADD_LOGGER(ProviderCheck)

    void thread_func_1();
    void thread_func_2();

    std::thread thread_1;
    std::thread thread_2;

    bool exit_ready;

    std::string demoAppAttribString1_Current;
    std::string demoAppAttribString2_Current;

    std::mutex mutex_method1; // Providers need to be threadsafe
    std::mutex mutex_method2; // Providers need to be threadsafe

    int thread_delay_time;
};

#endif // PROVIDERCHECK_H
