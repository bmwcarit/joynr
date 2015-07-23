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

#include "cluster-controller/capabilities-client/FakeCapabilitiesClient.h"
#include "joynr/exceptions.h"
#include "cluster-controller/capabilities-client/IGlobalCapabilitiesCallback.h"

#include <QUuid>

namespace joynr
{

FakeCapabilitiesClient::FakeCapabilitiesClient(const std::string& localChannelId,
                                               const QString& settingsFileName)
        : defaultRequestTTL(30000),
          defaultRequestRoundtripTTL(40000),
          capabilitiesClientParticipantId(),
          localChannelId(localChannelId),
          configuration(settingsFileName, QSettings::IniFormat),
          preconfiguredDomain(configuration.value("capabilitiesClient/fakeDomain", "fakeDomain")
                                      .value<QString>()
                                      .toStdString()),
          preconfiguredInterfaceName(
                  configuration.value("capabilitiesClient/fakeInterfaceName", "fakeInterfaceName")
                          .value<QString>()
                          .toStdString()),
          preconfiguredChannelId(
                  configuration.value("capabilitiesClient/fakeChannelId", "fakeChannelId")
                          .value<QString>()
                          .toStdString()),
          preconfiguredParticipantId(
                  configuration.value("capabilitiesClient/fakeParticipantId", "fakeParticipantId")
                          .value<QString>()
                          .toStdString())
{
    QString uuid = QUuid::createUuid().toString();
    capabilitiesClientParticipantId = uuid.mid(1, uuid.length() - 2).toStdString();
}

FakeCapabilitiesClient::~FakeCapabilitiesClient()
{
}

std::string FakeCapabilitiesClient::getLocalChannelId()
{
    return localChannelId;
}

void FakeCapabilitiesClient::add(
        std::vector<types::StdCapabilityInformation> capabilitiesInformationList)
{
    Q_UNUSED(capabilitiesInformationList)
    if (localChannelId.empty()) {
        throw JoynrException("Exception in CapabilitiesClient: Local channelId is empty. Tried to "
                             "register capabilities before messaging was started(no queueing "
                             "implemented yet)");
    } else {
    }
}

void FakeCapabilitiesClient::remove(std::vector<std::string> participantIdList)
{
    Q_UNUSED(participantIdList);
}

std::vector<types::StdCapabilityInformation> FakeCapabilitiesClient::lookup(
        const std::string& domain,
        const std::string& interfaceName)
{
    // return faked list to simulate incoming results
    return createFakedCapInfoList(domain, interfaceName);
}

void FakeCapabilitiesClient::lookup(const std::string& domain,
                                    const std::string& interfaceName,
                                    QSharedPointer<IGlobalCapabilitiesCallback> callback)
{
    // return faked list to simulate incoming results
    callback->capabilitiesReceived(createFakedCapInfoList(domain, interfaceName));
}

void FakeCapabilitiesClient::lookup(const std::string& participantId,
                                    QSharedPointer<IGlobalCapabilitiesCallback> callback)
{
    callback->capabilitiesReceived(createFakedCapInfoListForParticipantId(participantId));
}

std::vector<types::StdCapabilityInformation> FakeCapabilitiesClient::createFakedCapInfoList(
        const std::string& domain,
        const std::string& interfaceName)
{
    std::vector<types::StdCapabilityInformation> fakedCapInfoList;
    std::string fakeParticipantId =
            preconfiguredParticipantId + "_" + interfaceName + "DummyProvider";
    fakedCapInfoList.push_back(types::StdCapabilityInformation(
            domain,
            interfaceName,
            types::StdProviderQos(std::vector<types::StdCustomParameter>(),
                                  1,
                                  1,
                                  types::StdProviderScope::GLOBAL,
                                  false),
            preconfiguredChannelId,
            fakeParticipantId));
    return fakedCapInfoList;
}

std::vector<types::StdCapabilityInformation> FakeCapabilitiesClient::
        createFakedCapInfoListForChannelId(const std::string& channelId)
{
    std::vector<types::StdCapabilityInformation> fakedCapInfoList;
    fakedCapInfoList.push_back(types::StdCapabilityInformation(
            preconfiguredDomain,
            preconfiguredInterfaceName,
            types::StdProviderQos(std::vector<types::StdCustomParameter>(),
                                  1,
                                  1,
                                  types::StdProviderScope::GLOBAL,
                                  false),
            channelId,
            preconfiguredParticipantId));
    return fakedCapInfoList;
}

std::vector<types::StdCapabilityInformation> FakeCapabilitiesClient::
        createFakedCapInfoListForParticipantId(const std::string& participantId)
{
    std::vector<types::StdCapabilityInformation> fakedCapInfoList;
    fakedCapInfoList.push_back(types::StdCapabilityInformation(
            preconfiguredDomain,
            preconfiguredInterfaceName,
            types::StdProviderQos(std::vector<types::StdCustomParameter>(),
                                  1,
                                  1,
                                  types::StdProviderScope::GLOBAL,
                                  false),
            preconfiguredChannelId,
            participantId));
    return fakedCapInfoList;
}

std::vector<types::StdCapabilityInformation> FakeCapabilitiesClient::createFakedCapInfoList()
{
    return createFakedCapInfoList(preconfiguredDomain, preconfiguredInterfaceName);
}

} // namespace joynr
