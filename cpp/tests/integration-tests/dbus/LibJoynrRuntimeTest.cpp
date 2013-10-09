/*
 * #%L
 * joynr::C++
 * $Id:$
 * $HeadURL:$
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

#include "joynr/IDbusSkeletonWrapper.h"
#include "joynr/DbusMessagingSkeleton.h"
#include "joynr/DbusCapabilitiesSkeleton.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "common/dbus/DbusMessagingEndpointAddress.h"
#include "tests/utils/MockObjects.h"

#include "common/dbus/DbusMessagingStubAdapter.h"

using namespace joynr;

class LibJoynrRuntimeTest : public testing::Test {

public:
    QString libjoynrSettingsFilename;

    QSettings* settings;

    LibJoynrRuntime* runtime;

    IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>* msgSkeleton;
    MockMessaging* msgMock;

    IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>* capSkeleton;
    MockCapabilitiesStub* capMock;

    LibJoynrRuntimeTest():
        libjoynrSettingsFilename("resources/libjoynrintegrationtest.settings"),
        settings(new QSettings(libjoynrSettingsFilename, QSettings::IniFormat)),
        runtime(NULL),
        msgSkeleton(NULL),
        capSkeleton(NULL)
    {
    }

    ~LibJoynrRuntimeTest() {
        settings->deleteLater();
    }

    void SetUp() {
        DbusSettings* dbusSettings = new DbusSettings(*settings);
        // start skeletons
        QString ccMessagingAddress(dbusSettings->getClusterControllerMessagingAddress());
        msgMock = new MockMessaging();
        msgSkeleton = new IDbusSkeletonWrapper<DbusMessagingSkeleton, IMessaging>(*msgMock, ccMessagingAddress);

        QString ccCapabilitiesAddress(dbusSettings->getClusterControllerCapabilitiesAddress());
        capMock = new MockCapabilitiesStub();
        capSkeleton = new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*capMock, ccCapabilitiesAddress);

        // start libjoynr runtime
        runtime = new LibJoynrRuntime(new QSettings(libjoynrSettingsFilename, QSettings::IniFormat));

        delete dbusSettings;
    }

    void TearDown() {
        delete runtime;
        delete msgSkeleton;
        delete msgMock;
        delete capSkeleton;
        delete capMock;
    }
};

TEST_F(LibJoynrRuntimeTest, instantiate_Runtime)
{
    ASSERT_TRUE(runtime != NULL);
}

TEST_F(LibJoynrRuntimeTest, get_proxy) {
    // create default entry
    QList<CapabilityEntry>* result = new QList<CapabilityEntry>();
    types::ProviderQos* pQos = new types::ProviderQos();
    pQos->setPriority(5);
    QSharedPointer<JoynrMessagingEndpointAddress> endPoint(new JoynrMessagingEndpointAddress("defaultChannelId"));
    CapabilityEntry* defaultEntry = new CapabilityEntry();
    defaultEntry->setParticipantId("defaultDbusId");
    defaultEntry->setQos(*pQos);
    defaultEntry->prependEndpointAddress(endPoint);
    result->append(*defaultEntry);
    EXPECT_CALL(*capMock, lookup(A<const QString&>(),
                                  A<const QString&>(),
                                  A<const types::ProviderQosRequirements&>(),
                                  A<const DiscoveryQos&>())).Times(1).WillRepeatedly(testing::Return(*result));
    EXPECT_CALL(*capMock, addEndpoint( A<const QString &>(),
                                       A<QSharedPointer<EndpointAddressBase> >(),
                                       A<const qint64& >())).Times(1);

    QString domain("localdomain");
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
    EXPECT_CALL(*capMock, add(A<const QString&>(),
                               A<const QString&>(),
                               A<const QString&>(),
                               A<const types::ProviderQos&>(),
                               A<QList<QSharedPointer<EndpointAddressBase> >>(),
                               A<QSharedPointer<EndpointAddressBase>>(),
                               A<const qint64&>())).Times(1);

    QString domain("localdomain");
    QString authenticationToken("authToken");

    types::ProviderQos providerQos;
    providerQos.setPriority(QDateTime::currentDateTime().toMSecsSinceEpoch());

    QSharedPointer<vehicle::GpsProvider> provider(new vehicle::DefaultGpsProvider(providerQos));

    QString participantId = runtime->registerCapability(domain, provider, authenticationToken);
    ASSERT_TRUE(participantId != NULL);

    runtime->unregisterCapability(participantId);
}
