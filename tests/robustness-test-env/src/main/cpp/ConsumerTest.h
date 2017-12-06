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

#ifndef CONSUMERTEST_H
#define CONSUMERTEST_H

#include "ConsumerProxy.h"

#include <string>
#include <vector>

#include "joynr/ISubscriptionListener.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"
#include "joynr/tests/robustness/TestInterfaceBroadcastSelectiveWithSingleStringParameter1BroadcastFilterParameters.h"
#include "joynr/tests/robustness/TestInterfaceBroadcastSelectiveWithSingleStringParameter2BroadcastFilterParameters.h"
#include "joynr/tests/robustness/TestInterfaceProxy.h"

template <class C>
class Wrap
{
public:
    Wrap() = default;

    Wrap(std::shared_ptr<C> ptr)
    {
        localSharedPointer = std::move(ptr);
    }

    C* operator->()
    {
        return localSharedPointer.get();
    }

    void reset()
    {
        localSharedPointer.reset();
    }

    C& operator*()
    {
        return *localSharedPointer;
    }

    Wrap& operator=(const Wrap&) = delete;
    Wrap(const Wrap&) = delete;

    Wrap& operator=(Wrap&& other) = default;
    Wrap(Wrap&& other) = default;

private:
    std::shared_ptr<C> localSharedPointer{nullptr};
};

// Container for multiple runtime/proxybuilders/proxies
struct proxyContainer {

    // proxy
    Wrap<joynr::tests::robustness::TestInterfaceProxy> proxy;
    std::unique_ptr<ConsumerProxy> myProxy;

    // broadcast1
    std::string myBroadcastSubscriptionId1;

    // broadcast2
    std::string myBroadcastSubscriptionId2;

    // selective broadcast 1
    std::string mySelectiveBroadcastSubscriptionId1;
    std::string filterParamKey1;
    std::string filterParamValue1 = "FilterForSelectiveBroadcast1";

    // selective broadcast 2
    std::string mySelectiveBroadcastSubscriptionId2;
    std::string filterParamKey2;
    std::string filterParamValue2 = "FilterForSelectiveBroadcast2";
};

struct proxyBuilderContainer {

    Wrap<joynr::ProxyBuilder<joynr::tests::robustness::TestInterfaceProxy>> proxyBuilder;
    std::vector<proxyContainer> proxyContainerList;
};

struct runTimeContainer {

    Wrap<joynr::JoynrRuntime> runtime;
    std::vector<proxyBuilderContainer> proxyBuilderContainerList;
};

// test case type
enum testCaseType {
    TEST_1 = 1,
    TEST_2,
    TEST_3,
};

struct ConsumerTestParameters {

    std::string pathToLibJoynrSettings;
    std::string pathToMessagingSettings;
    std::string providerDomain;
    int threadDelayMS;
    int numOfRuntimes;
    int numOfProxyBuilders;
    int numOfProxies;
};

class ConsumerTest
{

public:
    ConsumerTest(const ConsumerTestParameters& params);
    ~ConsumerTest();
    void init();
    void shutDown(unsigned int testCase);

private:
    ADD_LOGGER(ConsumerTest)
    void runTests();
    void stopTests();
    void unsubscribeFromAll();
    void destroyAllRuntimes();
    void destroyAllProxyBuilders();
    void destroyAllProxies();
    void destroyAllContainers();

    ConsumerTestParameters testParams;
    std::vector<runTimeContainer> myRunTimeContainerList;
};

#endif // CONSUMERTEST_H
