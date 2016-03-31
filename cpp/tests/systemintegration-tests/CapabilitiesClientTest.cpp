/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <string>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "cluster-controller/capabilities-client/CapabilitiesClient.h"
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "cluster-controller/messaging/MessagingPropertiesPersistence.h"
#include "joynr/TypeUtil.h"
#include "joynr/types/Version.h"

using namespace ::testing;
using namespace joynr;

ACTION_P(ReleaseSemaphore,semaphore)
{
    semaphore->notify();
}

static const std::string messagingPropertiesPersistenceFileName("CapabilitiesClientTest-joynr.settings");
static const std::string libJoynrSettingsFilename("test-resources/libjoynrSystemIntegration1.settings");

class CapabilitiesClientTest : public TestWithParam< std::string > {
public:
    ADD_LOGGER(CapabilitiesClientTest);
    JoynrClusterControllerRuntime* runtime;
    Settings *settings;
    MessagingSettings messagingSettings;
    std::string channelId;

    CapabilitiesClientTest() :
        runtime(nullptr),
        settings(new Settings(GetParam())),
        messagingSettings(*settings)
    {
        messagingSettings.setMessagingPropertiesPersistenceFilename(messagingPropertiesPersistenceFileName);
        MessagingPropertiesPersistence storage(messagingSettings.getMessagingPropertiesPersistenceFilename());
        channelId = storage.getChannelId();
        Settings libjoynrSettings{libJoynrSettingsFilename};
        Settings::merge(libjoynrSettings, *settings, false);

        runtime = new JoynrClusterControllerRuntime(nullptr, settings);
    }

    void SetUp() {
        runtime->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime->stop(deleteChannel);
    }

    ~CapabilitiesClientTest(){
        delete runtime;
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClientTest);

};

INIT_LOGGER(CapabilitiesClientTest);

TEST_P(CapabilitiesClientTest, registerAndRetrieveCapability) {
    auto capabilitiesClient = std::make_unique<CapabilitiesClient>(channelId);
    ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>* capabilitiesProxyBuilder =
            runtime->createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                messagingSettings.getDiscoveryDirectoriesDomain()
            );
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY); //actually only one provider should be available
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> cabilitiesProxy (
        capabilitiesProxyBuilder
            ->setMessagingQos(MessagingQos(10000)) //TODO magic values.
            ->setCached(true)
            ->setDiscoveryQos(discoveryQos)
            ->build()
        );
    capabilitiesClient->init(cabilitiesProxy);

    std::vector<types::CapabilityInformation> capabilitiesInformationList;
    std::string capDomain("testDomain");
    std::string capInterface("testInterface");
    types::ProviderQos capProviderQos;
    std::string capChannelId("testChannelId");
    std::string capParticipantId("testParticipantId");
    joynr::types::Version providerVersion(47, 11);

    capabilitiesInformationList.push_back(types::CapabilityInformation(providerVersion,
        capDomain, capInterface, capProviderQos, capChannelId, capParticipantId));
    JOYNR_LOG_DEBUG(logger, "Registering capabilities");
    capabilitiesClient->add(capabilitiesInformationList);
    JOYNR_LOG_DEBUG(logger, "Registered capabilities");
    //sync methods are not yet implemented
//    std::vector<types::CapabilityInformation> capResultList = capabilitiesClient->lookup(capDomain, capInterface);
//    EXPECT_EQ(capResultList, capabilitiesInformationList);
    auto callback = std::make_shared<GlobalCapabilitiesMock>();

    // use a semaphore to wait for capabilities to be received
    Semaphore semaphore(0);
    EXPECT_CALL(*callback, capabilitiesReceived(A<const std::vector<types::CapabilityInformation>&>()))
           .WillRepeatedly(
                DoAll(
                    ReleaseSemaphore(&semaphore),
                    Return()
                ));
    std::function<void(const std::vector<types::CapabilityInformation>&)> onSuccess =
            [&](const std::vector<types::CapabilityInformation>& capabilities) {
                callback->capabilitiesReceived(capabilities);
            };

    JOYNR_LOG_DEBUG(logger, "get capabilities");
    capabilitiesClient->lookup(capDomain, capInterface, onSuccess);
    semaphore.waitFor(std::chrono::seconds(10));
    JOYNR_LOG_DEBUG(logger, "finished get capabilities");
    delete capabilitiesProxyBuilder;
}

INSTANTIATE_TEST_CASE_P(Http,
        CapabilitiesClientTest,
        testing::Values(
            "test-resources/HttpSystemIntegrationTest1.settings"
        )
);

INSTANTIATE_TEST_CASE_P(MqttWithHttpBackend,
        CapabilitiesClientTest,
        testing::Values(
            "test-resources/MqttWithHttpBackendSystemIntegrationTest1.settings"
        )
);
