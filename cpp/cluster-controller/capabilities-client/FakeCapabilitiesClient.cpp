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

namespace joynr {

FakeCapabilitiesClient::FakeCapabilitiesClient(const QString& localChannelId,
                                               const QString &settingsFileName)
    : defaultRequestTTL(30000),
      defaultRequestRoundtripTTL(40000),
      capabilitiesClientParticipantId(),
      localChannelId(localChannelId),
      configuration(settingsFileName, QSettings::IniFormat),
      preconfiguredDomain(configuration
                          .value("capabilitiesClient/fakeDomain", "fakeDomain")
                          .value<QString>()),
      preconfiguredInterfaceName(configuration
                                 .value("capabilitiesClient/fakeInterfaceName", "fakeInterfaceName")
                                 .value<QString>()),
      preconfiguredChannelId(configuration
                             .value("capabilitiesClient/fakeChannelId", "fakeChannelId")
                             .value<QString>()),
      preconfiguredParticipantId(configuration
                                 .value("capabilitiesClient/fakeParticipantId", "fakeParticipantId")
                                 .value<QString>())
{
    capabilitiesClientParticipantId = QUuid::createUuid().toString();
    capabilitiesClientParticipantId = capabilitiesClientParticipantId
                                        .mid(1,capabilitiesClientParticipantId.length()-2);
}

FakeCapabilitiesClient::~FakeCapabilitiesClient(){
}


QString FakeCapabilitiesClient::getLocalChannelId(){
    return localChannelId;
}


void FakeCapabilitiesClient::add(QList<types::CapabilityInformation> capabilitiesInformationList){
    Q_UNUSED(capabilitiesInformationList)
    if (localChannelId.isEmpty()){
        throw JoynrException("Exception in CapabilitiesClient: Local channelId is empty. Tried to register capabilities before messaging was started(no queueing implemented yet)");
    } else {
    }
}

void FakeCapabilitiesClient::remove(QList<QString> participantIdList){
    Q_UNUSED(participantIdList);
}

QList<types::CapabilityInformation> FakeCapabilitiesClient::lookup(const QString& domain,
                                                                                          const QString& interfaceName)
{
    //return faked list to simulate incoming results
    return createFakedCapInfoList(domain, interfaceName);
}

void FakeCapabilitiesClient::lookup(const QString& domain,
                                                                const QString& interfaceName,
                                                                QSharedPointer<IGlobalCapabilitiesCallback> callback)
{
    //return faked list to simulate incoming results
        callback->capabilitiesReceived(createFakedCapInfoList(domain, interfaceName));
}


void FakeCapabilitiesClient::lookup(const QString& participantId,
                                                             QSharedPointer<IGlobalCapabilitiesCallback> callback)
{
    callback->capabilitiesReceived(createFakedCapInfoListForParticipantId(participantId));
}

QList<types::CapabilityInformation> FakeCapabilitiesClient::createFakedCapInfoList(const QString& domain,
                                                                              const QString& interfaceName)
{
    QList<types::CapabilityInformation> fakedCapInfoList;
    QString fakeParticipantId = preconfiguredParticipantId + "_" + interfaceName + "DummyProvider";
    fakedCapInfoList.append(types::CapabilityInformation(domain,
                                                    interfaceName,
                                                    types::ProviderQos(QList<types::CustomParameter>(),1,1,types::ProviderScope::GLOBAL,false),
                                                    preconfiguredChannelId,
                                                    fakeParticipantId));
    return fakedCapInfoList;
}

QList<types::CapabilityInformation> FakeCapabilitiesClient::createFakedCapInfoListForChannelId(const QString& channelId){
    QList<types::CapabilityInformation> fakedCapInfoList;
    fakedCapInfoList.append(types::CapabilityInformation(preconfiguredDomain,
                                                    preconfiguredInterfaceName,
                                                    types::ProviderQos(QList<types::CustomParameter>(),1,1,types::ProviderScope::GLOBAL,false),
                                                    channelId,
                                                    preconfiguredParticipantId));
    return fakedCapInfoList;
}

QList<types::CapabilityInformation> FakeCapabilitiesClient::createFakedCapInfoListForParticipantId(const QString& participantId){
    QList<types::CapabilityInformation> fakedCapInfoList;
    fakedCapInfoList.append(types::CapabilityInformation(preconfiguredDomain,
                                                    preconfiguredInterfaceName,
                                                    types::ProviderQos(QList<types::CustomParameter>(),1,1,types::ProviderScope::GLOBAL,false),
                                                    preconfiguredChannelId,
                                                    participantId));
    return fakedCapInfoList;
}

QList<types::CapabilityInformation> FakeCapabilitiesClient::createFakedCapInfoList(){
    return createFakedCapInfoList(preconfiguredDomain, preconfiguredInterfaceName);
}


} // namespace joynr
