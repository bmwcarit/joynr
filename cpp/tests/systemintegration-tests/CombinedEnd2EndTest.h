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

#ifndef COMBINEDEND2ENDTEST_H
#define COMBINEDEND2ENDTEST_H

#include <string>
#include <memory>

#include <gtest/gtest.h>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/MessagingSettings.h"
#include "joynr/Logger.h"
#include "joynr/Semaphore.h"
#include "joynr/Settings.h"

/*
 * This test creates two Runtimes and will test communication
 * between the two Runtimes via HttpReceiver
 *
 */
class CombinedEnd2EndTest : public testing::TestWithParam<std::tuple<std::string, std::string>>
{
public:
    CombinedEnd2EndTest();
    ~CombinedEnd2EndTest() override = default;

    void SetUp() override;
    void TearDown() override;
    std::shared_ptr<joynr::JoynrRuntime> runtime1;
    std::shared_ptr<joynr::JoynrRuntime> runtime2;
    std::string registeredSubscriptionId;
    std::string messagingSettingsFile1;
    std::string messagingSettingsFile2;
    joynr::Settings settings1;
    joynr::Settings settings2;
    joynr::MessagingSettings messagingSettings1;
    joynr::MessagingSettings messagingSettings2;
    std::string baseUuid;
    std::string uuid;
    std::string domainName;
    joynr::Semaphore semaphore;
    joynr::DiscoveryQos discoveryQos;

    ADD_LOGGER(CombinedEnd2EndTest)

private:
    DISALLOW_COPY_AND_ASSIGN(CombinedEnd2EndTest);
};

#endif // COMBINEDEND2ENDTEST_H
