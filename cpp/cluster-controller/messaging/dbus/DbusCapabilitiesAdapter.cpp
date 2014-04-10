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
#include "joynr/DbusCapabilitiesAdapter.h"
#include "joynr/system/ChannelAddress.h"

namespace joynr {

DbusCapabilitiesAdapter::DbusCapabilitiesAdapter(MessagingEndpointDirectory& messagingEndpointDirectory, LocalCapabilitiesDirectory& localCapabilitiesDirectory, QString dbusAddress, QString ccChannelId)
    : dbusSkeletonWrapper(new IDbusSkeletonWrapper<DbusCapabilitiesSkeleton, ICapabilities>(*this, dbusAddress)),
      messagingEndpointDirectory(messagingEndpointDirectory),
      localCapabilitiesDirectory(localCapabilitiesDirectory),
      ccChannelID(ccChannelId)
{
}

DbusCapabilitiesAdapter::~DbusCapabilitiesAdapter() {
    delete dbusSkeletonWrapper;
}

void DbusCapabilitiesAdapter::add(
        const QString& domain,
        const QString& interfaceName,
        const QString& participantId,
        const types::ProviderQos& qos,
        QList<QSharedPointer<joynr::system::Address> > endpointAddressList,
        QSharedPointer<joynr::system::Address> messagingStubAddress,
        const qint64& timeout_ms
){
    Q_UNUSED(timeout_ms); //timeout does not make sense for in-process communication, but has to be in the method signature to fullfill common interface
    dbusSkeletonWrapper->logMethodCall("add", "DbusCapabilitiesAdapter");
    messagingEndpointDirectory.add(participantId, messagingStubAddress);
    assert(messagingStubAddress!=NULL);
    assert(!messagingStubAddress.isNull());
    // add the cc joynr messaging address to the endpoint address list
    QSharedPointer<system::ChannelAddress> ccEndpointAddr(new system::ChannelAddress(ccChannelID));
    endpointAddressList.append(ccEndpointAddr);
    localCapabilitiesDirectory.registerCapability(domain, interfaceName, qos, participantId, endpointAddressList); //TODO pass the timout
}

void DbusCapabilitiesAdapter::addEndpoint(
        const QString& participantId,
        QSharedPointer<joynr::system::Address> messagingStubAddress,
        const qint64& timeout_ms
){
    Q_UNUSED(timeout_ms); //timeout does not make sense for in-process communication, but has to be in the method signature to fullfill common interface
    dbusSkeletonWrapper->logMethodCall("addEndpoint", "DbusCapabilitiesAdapter");
    messagingEndpointDirectory.add(participantId, messagingStubAddress);
}

QList<CapabilityEntry> DbusCapabilitiesAdapter::lookup(
        const QString& domain,
        const QString& interfaceName,
        const DiscoveryQos& discoveryQos
){
    dbusSkeletonWrapper->logMethodCall("lookup", "DbusCapabilitiesAdapter");
    QSharedPointer<DummyDbusCapabilitiesFuture> future(new DummyDbusCapabilitiesFuture());
    joynr::system::DiscoveryQos newDiscoveryQos;
    newDiscoveryQos.setCacheMaxAge(discoveryQos.getCacheMaxAge());
    newDiscoveryQos.setProviderMustSupportOnChange(discoveryQos.getProviderMustSupportOnChange());
    newDiscoveryQos.setDiscoveryScope(discoveryQos.getDiscoveryScope());
    localCapabilitiesDirectory.getCapabilities(domain,interfaceName, future, newDiscoveryQos);
    //this will block forever when no result is received.
    return future->get();
}

QList<CapabilityEntry> DbusCapabilitiesAdapter::lookup(
        const QString& participantId,
        const DiscoveryQos& discoveryQos
) {
    dbusSkeletonWrapper->logMethodCall("lookup", "DbusCapabilitiesAdapter");
    QSharedPointer<DummyDbusCapabilitiesFuture> future(new DummyDbusCapabilitiesFuture());
    joynr::system::DiscoveryQos newDiscoveryQos;
    newDiscoveryQos.setCacheMaxAge(discoveryQos.getCacheMaxAge());
    newDiscoveryQos.setProviderMustSupportOnChange(discoveryQos.getProviderMustSupportOnChange());
    newDiscoveryQos.setDiscoveryScope(discoveryQos.getDiscoveryScope());
    localCapabilitiesDirectory.getCapabilities(participantId, future, newDiscoveryQos);
    return future->get();
}

void DbusCapabilitiesAdapter::remove(const QString& participantId, const qint64& timeout_ms) {
    dbusSkeletonWrapper->logMethodCall("remove", "DbusCapabilitiesAdapter");
    localCapabilitiesDirectory.removeCapability(participantId);
    messagingEndpointDirectory.remove(participantId);
    assert(timeout_ms<1); // timeout is not yet supported for removing.
}

/***************************************/
/***************************************/

DummyDbusCapabilitiesFuture::DummyDbusCapabilitiesFuture()
    : futureSemaphore(0),
      capabilities()
{
}

void DummyDbusCapabilitiesFuture::capabilitiesReceived(QList<CapabilityEntry> capabilities){
    this->capabilities = capabilities;
    futureSemaphore.release(1);
}

QList<CapabilityEntry> DummyDbusCapabilitiesFuture::get(){
    futureSemaphore.acquire(1);
    futureSemaphore.release(1);
    return capabilities;
}

QList<CapabilityEntry> DummyDbusCapabilitiesFuture::get(const qint64& timeout){

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
