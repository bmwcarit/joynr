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

#include "ProviderTest.h"

#include "joynr/types/ProviderQos.h"

using joynr::JoynrRuntime;

void ProviderTest::init()
{
    // onFatalRuntimeError callback is optional, but it is highly recommended to provide an
    // implementation.
    std::function<void(const joynr::exceptions::JoynrRuntimeException&)> onFatalRuntimeError =
            [&] (const joynr::exceptions::JoynrRuntimeException& exception) {
        JOYNR_LOG_ERROR(logger(), "Unexpected joynr runtime error occured: " + exception.getMessage());
    };

    for(int i=0; i < input.numOfRuntimes; i++)
    {
        runTimeContainer runTimeContainerTmp;
        runTimeContainerTmp.runtime = JoynrRuntime::createRuntime(input.pathToLibJoynrSettings, onFatalRuntimeError, input.pathToMessagingSettings);
        myRunTimeContainerList.push_back(runTimeContainerTmp);
    }

    // default uses a priority that is the current time,
    // causing arbitration to the last started instance if highest priority arbitrator is used
    std::chrono::milliseconds millisSinceEpoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    joynr::types::ProviderQos robustnessProviderQos;
    robustnessProviderQos.setPriority(millisSinceEpoch.count());
    robustnessProviderQos.setScope(joynr::types::ProviderScope::GLOBAL);
    robustnessProviderQos.setSupportsOnChangeSubscriptions(true);

    // register the runtimes
    for(int iRunTimeCount=0; iRunTimeCount < input.numOfRuntimes; iRunTimeCount++)
    {
        for(int iProvCount=0; iProvCount < input.numOfProviders; iProvCount++)
        {
            providerContainer providerContainerTmp;
            std::shared_ptr<JoynrRuntime> runtimeTmp = myRunTimeContainerList[iRunTimeCount].runtime;
            providerContainerTmp.provider = std::make_shared<ProviderCheck>(input.threadDelayMS);
            providerContainerTmp.domainName = input.providerDomain + "_" + std::to_string(iRunTimeCount) + "_" + std::to_string(iProvCount);
            providerContainerTmp.participantId = runtimeTmp->registerProvider<joynr::tests::robustness::TestInterfaceProvider>
                    (providerContainerTmp.domainName, providerContainerTmp.provider, robustnessProviderQos);

            JOYNR_LOG_INFO(logger(), "Provider is registered. ParticipantId: {}", providerContainerTmp.participantId);

            myRunTimeContainerList[iRunTimeCount].providerContainerList.push_back(providerContainerTmp);
        }
    }
}

void ProviderTest::shutdown()
{
    for(auto& it_runtime : myRunTimeContainerList) {
        for(auto& it_provider : it_runtime.providerContainerList) {

            std::string participantIdTmp;

            if((!it_runtime.runtime) || (!it_provider.provider))
                continue;

            participantIdTmp = it_runtime.runtime->unregisterProvider<joynr::tests::robustness::TestInterfaceProvider>( it_provider.domainName, it_provider.provider );
            JOYNR_LOG_INFO(logger(), "unregisterProvider finished. ParticipantId: {}", participantIdTmp);
        }
        it_runtime.providerContainerList.erase(it_runtime.providerContainerList.begin(), it_runtime.providerContainerList.end());
    }
    myRunTimeContainerList.erase(myRunTimeContainerList.begin(), myRunTimeContainerList.end());
}
