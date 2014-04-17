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
#include "joynr/JoynrRuntime.h"
#include "JoynrClusterControllerRuntime.h"
#include "joynr/SettingsMerger.h"

namespace joynr {

JoynrRuntime::JoynrRuntime(QSettings &settings) :
        proxyFactory(NULL),
        joynrCapabilitiesSendStub(NULL),
        participantIdStorage(NULL),
        capabilitiesRegistrar(NULL),
        capabilitiesAggregator(NULL),
        systemServicesSettings(settings),
        dispatcherAddress(NULL),
        messageRouter(NULL),
        discoveryProxy(NULL)
{
    systemServicesSettings.printSettings();
}


JoynrRuntime* JoynrRuntime::createRuntime(const QString& pathToLibjoynrSettings,
                                            const QString& pathToMessagingSettings) {
    QSettings* settings = SettingsMerger::mergeSettings(pathToLibjoynrSettings);
    SettingsMerger::mergeSettings(pathToMessagingSettings, settings);
    return JoynrClusterControllerRuntime::create(settings);
}

joynr::system::DiscoveryProxy* JoynrRuntime::createDiscoveryProxy() {
    QString systemServicesDomain = systemServicesSettings.getDomain();
    QString discoveryProviderParticipantId = systemServicesSettings.getCcDiscoveryProviderParticipantId();

    DiscoveryQos discoveryProviderDiscoveryQos;
    discoveryProviderDiscoveryQos.setCacheMaxAge(1000);
    discoveryProviderDiscoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::FIXED_PARTICIPANT);
    discoveryProviderDiscoveryQos.addCustomParameter("fixedParticipantId", discoveryProviderParticipantId);
    discoveryProviderDiscoveryQos.setDiscoveryTimeout(1000);

    ProxyBuilder<joynr::system::DiscoveryProxy>* discoveryProxyBuilder =
            getProxyBuilder<joynr::system::DiscoveryProxy>(systemServicesDomain);
    joynr::system::DiscoveryProxy* discoveryProxy = discoveryProxyBuilder
            ->setRuntimeQos(MessagingQos(5000))
            ->setCached(false)
            ->setDiscoveryQos(discoveryProviderDiscoveryQos)
            ->build();
    delete discoveryProxyBuilder;
    return discoveryProxy;
}

} // namespace joynr
