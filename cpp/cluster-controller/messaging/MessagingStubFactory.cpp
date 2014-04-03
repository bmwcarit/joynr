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
#include "joynr/MessagingStubFactory.h"
#include "common/in-process/InProcessMessagingStub.h"
#include "libjoynr/in-process/InProcessMessagingEndpointAddress.h"
#include "libjoynr/some-ip/SomeIpEndpointAddress.h"
#include "cluster-controller/messaging/joynr-messaging/JoynrMessagingStub.h"

#include "joynr/IMessaging.h"
#include "joynr/JoynrMessagingEndpointAddress.h"
#include "cluster-controller/messaging/in-process/InProcessClusterControllerMessagingSkeleton.h"
#include "joynr/ICommunicationManager.h"

#include "joynr/RuntimeConfig.h"
#include "libjoynr/dbus/DbusMessagingEndpointAddress.h"
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
#include "common/dbus/DbusMessagingStubAdapter.h"
#endif // USE_DBUS_COMMONAPI_COMMUNICATION

#include <cassert>

namespace joynr {

MessagingStubFactory::~MessagingStubFactory() {}

MessagingStubFactory::MessagingStubFactory(ICommunicationManager &comMgr) :
    partId2MessagingStubDirectory(QString("MessagingStubFactory-MessagingStubDirectory")),
    communicationManager(comMgr)
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    ,dbusMessagingStubDirectory("dbusMessagingStubDirectory")
 #endif // USE_DBUS_COMMONAPI_COMMUNICATION
{
}

QSharedPointer<IMessaging> MessagingStubFactory::create(
        QString destParticipantId,
        QSharedPointer <joynr::system::Address> destinationAddress) {

    if (partId2MessagingStubDirectory.contains(destParticipantId)) {
        return partId2MessagingStubDirectory.lookup(destParticipantId);
    }
    QSharedPointer<IMessaging> stub;
    if(isLocal(destParticipantId)) {}

    if(isJoynr(destinationAddress)) { // make a sendstub that uses the communicationManager so send it!
        QSharedPointer<JoynrMessagingEndpointAddress> joynrAddress = destinationAddress
                .dynamicCast<JoynrMessagingEndpointAddress>();
        stub = QSharedPointer<IMessaging>(new JoynrMessagingStub(communicationManager, joynrAddress->getChannelId()));
        assert (!stub.isNull());
    }
    if(isInProcessMessaging(destinationAddress)) {
        QSharedPointer<InProcessMessagingEndpointAddress> inProcessMessagingAddress =
                destinationAddress.dynamicCast<InProcessMessagingEndpointAddress>();
        stub = QSharedPointer<IMessaging>(new InProcessMessagingStub(inProcessMessagingAddress->getSkeleton()));
        assert (!stub.isNull());
    }
    if(isSomeIp(destinationAddress)) {}

    if(isDbus(destinationAddress)) {
#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
        QSharedPointer<DbusMessagingEndpointAddress> dbusAddress = destinationAddress.dynamicCast<DbusMessagingEndpointAddress>();
        QString address = dbusAddress->getServiceAddress();
        // lookup address
        if(dbusMessagingStubDirectory.contains(address)) {
            stub = dbusMessagingStubDirectory.lookup(address);
        } else {
            // create new stub
            stub = QSharedPointer<IMessaging>(new DbusMessagingStubAdapter(address));
            dbusMessagingStubDirectory.add(address, stub);
        }
#else
        assert(false);
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
    }

    assert (!stub.isNull());

    // QSharedPointer <IMessaging> messagingStub(stub);
    partId2MessagingStubDirectory.add(destParticipantId, stub);
    return stub;
}

void MessagingStubFactory::remove(QString destParticipantId) {
    partId2MessagingStubDirectory.remove(destParticipantId);
}

bool MessagingStubFactory::contains(QString destParticipantId) {
    return partId2MessagingStubDirectory.contains(destParticipantId);
}

bool MessagingStubFactory::isInProcessMessaging(QSharedPointer<joynr::system::Address> destinationAddress){
    if (destinationAddress->metaObject()->className() == InProcessMessagingEndpointAddress::ENDPOINT_ADDRESS_TYPE()) {
        return true;
    }
    return false;
}

bool MessagingStubFactory::isJoynr(QSharedPointer <joynr::system::Address> destinationAddress) {
    if(destinationAddress->metaObject()->className() == JoynrMessagingEndpointAddress::ENDPOINT_ADDRESS_TYPE()) {
        return true;
    }
    return false;
}

bool MessagingStubFactory::isSomeIp(QSharedPointer <joynr::system::Address> destinationAddress) {
    if( destinationAddress->metaObject()->className() == SomeIpEndpointAddress::ENDPOINT_ADDRESS_TYPE() ) {
        return true;
    }
    return false;
}

bool MessagingStubFactory::isLocal(QString destParticipantId) {
    Q_UNUSED(destParticipantId)
    return false;
}

bool MessagingStubFactory::isDbus(QSharedPointer<joynr::system::Address> destinationAddress) {
    if (destinationAddress->metaObject()->className() == DbusMessagingEndpointAddress::ENDPOINT_ADDRESS_TYPE()) {
        return true;
    }
    return false;
}


} // namespace joynr
