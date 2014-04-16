/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include <gtest/gtest.h>
#include "runtimes/libjoynr-runtime/LibJoynrRuntime.h"
#include "joynr/MessagingSettings.h"
#include "joynr/LibjoynrSettings.h"
#include "common/dbus/DbusSettings.h"
#include "joynr/vehicle/GpsProxy.h"
#include "joynr/vehicle/DefaultGpsProvider.h"
#include "joynr/SystemServicesSettings.h"

#include "joynr/IDbusSkeletonWrapper.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/DbusCapabilitiesSkeleton.h"
#include "joynr/system/ChannelAddress.h"
#include "joynr/system/CommonApiDbusAddress.h"
#include "tests/utils/MockObjects.h"

#include "common/dbus/DbusMessagingStubAdapter.h"

using namespace joynr;

class LibJoynrRuntimeTest : public testing::Test {

public:
    QString libjoynrSettingsFilename;
    QSettings settings;
    DbusSettings dbusSettings;
    LibJoynrRuntime* runtime;

    MockMessaging mockMessaging;
    IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging> msgSkeleton;

    MockCapabilitiesStub mockCapabilitiesStub;
    IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities> capSkeleton;

    LibJoynrRuntimeTest() :
            libjoynrSettingsFilename("test-resources/libjoynrintegrationtest.settings"),
            settings(libjoynrSettingsFilename, QSettings::IniFormat),
            dbusSettings(settings),
            runtime(NULL),
            mockMessaging(),
            msgSkeleton(
                mockMessaging,
                dbusSettings.createClusterControllerMessagingAddressString()
            ),
            mockCapabilitiesStub(),
            capSkeleton(
                mockCapabilitiesStub,
                dbusSettings.createClusterControllerCapabilitiesAddressString()
            )
    {
        SystemServicesSettings systemServicesSettings(settings);
        // provision entry to arbitrate routing provider
        QString routingProviderParticipantId(systemServicesSettings.getCcRoutingProviderParticipantId());
        QList<CapabilityEntry> routingCapabilityEntries;
        types::ProviderQos pQos;
        pQos.setPriority(5);
        QSharedPointer<system::ChannelAddress> routingProviderAddress(
                    new system::ChannelAddress("LibJoynrRuntimeTest.ChannelId")
        );
        CapabilityEntry routingProviderCapabilityEntry;
        routingProviderCapabilityEntry.setParticipantId(routingProviderParticipantId);
        routingProviderCapabilityEntry.setQos(pQos);
        routingProviderCapabilityEntry.prependEndpointAddress(routingProviderAddress);
        routingCapabilityEntries.append(routingProviderCapabilityEntry);
        EXPECT_CALL(
                    mockCapabilitiesStub,
                    lookup(routingProviderParticipantId, A<const DiscoveryQos&>())
        )
                    .WillRepeatedly(testing::Return(routingCapabilityEntries));
    }

    ~LibJoynrRuntimeTest() {
    }

    void SetUp() {
        // start libjoynr runtime
        runtime = new LibJoynrRuntime(new QSettings(libjoynrSettingsFilename, QSettings::IniFormat));
    }

    void TearDown() {
        delete runtime;
    }
};

TEST_F(LibJoynrRuntimeTest, instantiate_Runtime)
{
    ASSERT_TRUE(runtime != NULL);
}

TEST_F(LibJoynrRuntimeTest, get_proxy) {
    QString domain("LibJoynrRuntimeTest.Domain.A");
    QString interface(vehicle::GpsProxy::getInterfaceName());

    // create default entry
    QList<CapabilityEntry> capabilityEntries;
    types::ProviderQos pQos;
    pQos.setPriority(5);
    QSharedPointer<system::ChannelAddress> defaultChannelAddress(
                new system::ChannelAddress("LibJoynrRuntimeTest.ChannelId.A")
    );
    CapabilityEntry defaultEntry;
    defaultEntry.setParticipantId("LibJoynrRuntimeTest.ParticipantId.A");
    defaultEntry.setQos(pQos);
    defaultEntry.prependEndpointAddress(defaultChannelAddress);
    capabilityEntries.append(defaultEntry);
    EXPECT_CALL(
                mockCapabilitiesStub,
                lookup(domain, interface, A<const DiscoveryQos&>())
    )
            .Times(1)
            .WillRepeatedly(testing::Return(capabilityEntries));
    EXPECT_CALL(
                mockCapabilitiesStub,
                addEndpoint(
                    A<const QString &>(),
                    A<QSharedPointer<joynr::system::Address> >(),
                    A<const qint64& >()
                )
    )
            .Times(1);

    ProxyBuilder<vehicle::GpsProxy>* proxyBuilder = runtime->getProxyBuilder<vehicle::GpsProxy>(domain);
    ASSERT_TRUE(proxyBuilder != NULL);

    // start arbitration
    DiscoveryQos discoveryQos;
    discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
    discoveryQos.setDiscoveryTimeout(5000);
    proxyBuilder->setDiscoveryQos(discoveryQos);

    vehicle::GpsProxy* proxy = proxyBuilder->build();
    ASSERT_TRUE(proxy != NULL);

    delete proxyBuilder;
    delete proxy;
}

TEST_F(LibJoynrRuntimeTest, register_unregister_capability) {
    QString domain("LibJoynrRuntimeTest.Domain.A");
    QString interface(vehicle::GpsProxy::getInterfaceName());

    EXPECT_CALL(
                mockCapabilitiesStub,
                add(
                    domain,
                    interface,
                    A<const QString&>(),
                    A<const types::ProviderQos&>(),
                    A<QList<QSharedPointer<joynr::system::Address> >>(),
                    A<QSharedPointer<joynr::system::Address>>(),
                    A<const qint64&>()
               )
    )
            .Times(1);

    QString authenticationToken("authToken");

    types::ProviderQos providerQos;
    providerQos.setPriority(QDateTime::currentDateTime().toMSecsSinceEpoch());

    QSharedPointer<vehicle::GpsProvider> provider(new vehicle::DefaultGpsProvider(providerQos));

    QString participantId = runtime->registerCapability(domain, provider, authenticationToken);
    ASSERT_TRUE(participantId != NULL);

    runtime->unregisterCapability(participantId);
}
