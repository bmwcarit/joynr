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
#include "cluster-controller/messaging/in-process/InProcessCapabilitiesSkeleton.h"
#include "assert.h"
#include "joynr/system/ChannelAddress.h"
#include <limits>

namespace joynr {

using namespace joynr_logging;
Logger* InProcessCapabilitiesSkeleton::logger = Logging::getInstance()->getLogger("MSG", "InProcessCapabilitiesSkeleton");

InProcessCapabilitiesSkeleton::InProcessCapabilitiesSkeleton(IMessagingEndpointDirectory *endpointDirectory, LocalCapabilitiesDirectory *localCapabilitiesDirectory, QString ccChannelId)
    : messagingEndpointDirectory(endpointDirectory),
      localCapabilitiesDirectory(localCapabilitiesDirectory),
      ccChannelId(ccChannelId)
{

}

void InProcessCapabilitiesSkeleton::addEndpoint(const QString& participantId, QSharedPointer<joynr::system::Address> messagingStubAddress, const qint64& timeout_ms){
    Q_UNUSED(timeout_ms); //timeout does not make sense for in-process communication, but has to be in the method signature to fullfill common interface
    assert(messagingEndpointDirectory!=NULL);
    messagingEndpointDirectory->add(participantId, messagingStubAddress);
}

void InProcessCapabilitiesSkeleton::add(
        const QString &domain,
        const QString &interfaceName,
        const QString &participantId,
        const types::ProviderQos &qos,
        QList<QSharedPointer<joynr::system::Address> > endpointAddressList,
        QSharedPointer<joynr::system::Address> messagingStubAddress,
        const qint64& timeout_ms
) {
    Q_UNUSED(timeout_ms); //timeout does not make sense for in-process communication, but has to be in the method signature to fullfill common interface
    assert(messagingEndpointDirectory!=NULL);
    messagingEndpointDirectory->add(participantId, messagingStubAddress);
    assert(localCapabilitiesDirectory!=NULL);
    assert(messagingStubAddress!=NULL);
    assert(!messagingStubAddress.isNull());
    // add the cc joynr messaging address to the endpoint address list
    QSharedPointer<system::ChannelAddress> ccEndpointAddr(new system::ChannelAddress(ccChannelId));
    endpointAddressList.append(ccEndpointAddr);
    localCapabilitiesDirectory->registerCapability(domain, interfaceName, qos, participantId, endpointAddressList); //TODO pass the timout
}

QList<CapabilityEntry> InProcessCapabilitiesSkeleton::lookup(
        const QString &domain,
        const QString &interfaceName,
        const DiscoveryQos& discoveryQos
){
    QSharedPointer<DummyCapabilitiesFuture> future(new DummyCapabilitiesFuture());
    joynr::system::DiscoveryQos newDiscoveryQos;
    newDiscoveryQos.setCacheMaxAge(discoveryQos.getCacheMaxAge());
    newDiscoveryQos.setProviderMustSupportOnChange(discoveryQos.getProviderMustSupportOnChange());
    newDiscoveryQos.setDiscoveryScope(discoveryQos.getDiscoveryScope());
    localCapabilitiesDirectory->getCapabilities(domain,interfaceName, future, newDiscoveryQos);
    //this will block forever when no result is received.
    return future->get();
}

QList<CapabilityEntry> InProcessCapabilitiesSkeleton::lookup(
        const QString &participantId,
        const DiscoveryQos& discoveryQos
){
    QSharedPointer<DummyCapabilitiesFuture> future(new DummyCapabilitiesFuture());
    joynr::system::DiscoveryQos newDiscoveryQos;
    newDiscoveryQos.setCacheMaxAge(discoveryQos.getCacheMaxAge());
    newDiscoveryQos.setProviderMustSupportOnChange(discoveryQos.getProviderMustSupportOnChange());
    newDiscoveryQos.setDiscoveryScope(discoveryQos.getDiscoveryScope());
    localCapabilitiesDirectory->getCapabilities(participantId, future, newDiscoveryQos);
    return future->get();
}

void InProcessCapabilitiesSkeleton::remove(const QString& participantId, const qint64& timeout_ms){
    localCapabilitiesDirectory->removeCapability(participantId);
    messagingEndpointDirectory->remove(participantId);
    assert(timeout_ms<1);// timeout is not yet supported for removing.
}

DummyCapabilitiesFuture::DummyCapabilitiesFuture()
    : futureSemaphore(0),
      capabilities()
{
}

void DummyCapabilitiesFuture::capabilitiesReceived(QList<CapabilityEntry> capabilities){
    this->capabilities = capabilities;
    futureSemaphore.release(1);
}

QList<CapabilityEntry> DummyCapabilitiesFuture::get(){
    futureSemaphore.acquire(1);
    futureSemaphore.release(1);
    return capabilities;
}

QList<CapabilityEntry> DummyCapabilitiesFuture::get(const qint64& timeout){

    int timeout_int (timeout);
    //prevent overflow during conversion from qint64 to int
    int maxint = std::numeric_limits< int >::max();
    if (timeout>maxint) {
        timeout_int = maxint;
    }

    if (futureSemaphore.tryAcquire(1, timeout_int)){
        futureSemaphore.release(1);
    }
    return capabilities;
}

} // namespace joynr
