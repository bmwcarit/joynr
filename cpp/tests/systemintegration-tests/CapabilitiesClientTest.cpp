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
#include <memory>
#include <string>

#include "joynr/PrivateCopyAssign.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "tests/utils/MockObjects.h"
#include "runtimes/cluster-controller-runtime/JoynrClusterControllerRuntime.h"
#include "libjoynrclustercontroller/capabilities-client/CapabilitiesClient.h"
#include "joynr/infrastructure/IGlobalCapabilitiesDirectory.h"
#include "joynr/LocalCapabilitiesDirectory.h"
#include "libjoynrclustercontroller/messaging/MessagingPropertiesPersistence.h"
#include "joynr/types/Version.h"

#include "JoynrTest.h"

using namespace ::testing;
using namespace joynr;

static const std::string messagingPropertiesPersistenceFileName("CapabilitiesClientTest-joynr.settings");
static const std::string libJoynrSettingsFilename("test-resources/libjoynrSystemIntegration1.settings");

class CapabilitiesClientTest : public TestWithParam< std::string > {
public:
    ADD_LOGGER(CapabilitiesClientTest);
    std::unique_ptr<JoynrClusterControllerRuntime> runtime;
    std::unique_ptr<Settings> settings;
    MessagingSettings messagingSettings;

    CapabilitiesClientTest() :
        runtime(nullptr),
        settings(std::make_unique<Settings>(GetParam())),
        messagingSettings(*settings)
    {
        messagingSettings.setMessagingPropertiesPersistenceFilename(messagingPropertiesPersistenceFileName);
        MessagingPropertiesPersistence storage(messagingSettings.getMessagingPropertiesPersistenceFilename());
        Settings libjoynrSettings{libJoynrSettingsFilename};
        Settings::merge(libjoynrSettings, *settings, false);

        runtime = std::make_unique<JoynrClusterControllerRuntime>(std::move(settings));
    }

    void SetUp() {
        runtime->start();
    }

    void TearDown() {
        bool deleteChannel = true;
        runtime->stop(deleteChannel);
        // Delete persisted files
        std::remove(LibjoynrSettings::DEFAULT_LOCAL_CAPABILITIES_DIRECTORY_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_MESSAGE_ROUTER_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_SUBSCRIPTIONREQUEST_PERSISTENCE_FILENAME().c_str());
        std::remove(LibjoynrSettings::DEFAULT_PARTICIPANT_IDS_PERSISTENCE_FILENAME().c_str());
    }

private:
    DISALLOW_COPY_AND_ASSIGN(CapabilitiesClientTest);

};

INIT_LOGGER(CapabilitiesClientTest);

TEST_P(CapabilitiesClientTest, registerAndRetrieveCapability) {
    std::unique_ptr<ProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>> capabilitiesProxyBuilder =
            runtime->createProxyBuilder<infrastructure::GlobalCapabilitiesDirectoryProxy>(
                messagingSettings.getDiscoveryDirectoriesDomain()
            );

    DiscoveryQos discoveryQos(10000);
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryQos.addCustomParameter(
            "fixedParticipantId", messagingSettings.getCapabilitiesDirectoryParticipantId());
    std::shared_ptr<infrastructure::GlobalCapabilitiesDirectoryProxy> cabilitiesProxy (
        capabilitiesProxyBuilder
            ->setMessagingQos(MessagingQos(10000)) //TODO magic values.
            ->setDiscoveryQos(discoveryQos)
            ->build()
        );

    std::unique_ptr<CapabilitiesClient> capabilitiesClient (std::make_unique<CapabilitiesClient>());
    capabilitiesClient->setProxyBuilder(std::move(capabilitiesProxyBuilder));

    std::string capDomain("testDomain");
    std::string capInterface("testInterface");
    types::ProviderQos capProviderQos;
    std::string capParticipantId("testParticipantId");
    std::string capPublicKeyId("publicKeyId");
    joynr::types::Version providerVersion(47, 11);
    std::int64_t capLastSeenMs = 0;
    std::int64_t capExpiryDateMs = 1000;
    std::string capSerializedChannelAddress("testChannelId");
    types::GlobalDiscoveryEntry globalDiscoveryEntry(providerVersion,
                capDomain,
                capInterface,
                capParticipantId,
                capProviderQos,
                capLastSeenMs,
                capExpiryDateMs,
                capPublicKeyId,
                capSerializedChannelAddress);

    JOYNR_LOG_DEBUG(logger, "Registering capabilities");
    capabilitiesClient->add(globalDiscoveryEntry,
                            [](){},
                            [](const joynr::exceptions::JoynrRuntimeException& /*exception*/){});
    JOYNR_LOG_DEBUG(logger, "Registered capabilities");

    auto callback = std::make_shared<GlobalCapabilitiesMock>();

    // use a semaphore to wait for capabilities to be received
    Semaphore semaphore(0);
    EXPECT_CALL(*callback, capabilitiesReceived(A<const std::vector<types::GlobalDiscoveryEntry>&>()))
           .WillRepeatedly(
                DoAll(
                    ReleaseSemaphore(&semaphore),
                    Return()
                ));
    std::function<void(const std::vector<types::GlobalDiscoveryEntry>&)> onSuccess =
            [&](const std::vector<types::GlobalDiscoveryEntry>& capabilities) {
                callback->capabilitiesReceived(capabilities);
            };

    JOYNR_LOG_DEBUG(logger, "get capabilities");
    std::int64_t defaultDiscoveryMessageTtl = messagingSettings.getDiscoveryMessagesTtl();
    capabilitiesClient->lookup({capDomain}, capInterface, defaultDiscoveryMessageTtl, onSuccess);
    semaphore.waitFor(std::chrono::seconds(10));
    JOYNR_LOG_DEBUG(logger, "finished get capabilities");
}

INSTANTIATE_TEST_CASE_P(DISABLED_Http,
        CapabilitiesClientTest,
        testing::Values(
            "test-resources/HttpSystemIntegrationTest1.settings"
        )
);

INSTANTIATE_TEST_CASE_P(Mqtt,
        CapabilitiesClientTest,
        testing::Values(
            "test-resources/MqttSystemIntegrationTest1.settings"
        )
);
