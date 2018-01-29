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

#ifndef PROVIDERTEST_H
#define PROVIDERTEST_H

#include <vector>

#include "joynr/JoynrRuntime.h"
#include "joynr/Logger.h"

#include "ProviderCheck.h"

// Container for multiple runtime/providers
struct providerContainer {

    std::shared_ptr<ProviderCheck> provider;
    std::string domainName;
    std::string participantId;
};

struct runTimeContainer {

    std::shared_ptr<joynr::JoynrRuntime> runtime;
    std::vector<providerContainer> providerContainerList;
};

struct ProviderTestParameters {

    std::string pathToLibJoynrSettings;
    std::string pathToMessagingSettings;
    std::string providerDomain;
    int threadDelayMS;
    int numOfRuntimes;
    int numOfProviders;
};

class ProviderTest
{

public:
    ProviderTest(const ProviderTestParameters & params):input(params){}
    ~ProviderTest()=default;
    void init();
    void shutdown();

private:
    ADD_LOGGER(ProviderTest)

    ProviderTestParameters input;
    std::vector<runTimeContainer> myRunTimeContainerList;

};

#endif // PROVIDERTEST_H
