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
#ifndef MESSAGINGSTUBFACTORY_H
#define MESSAGINGSTUBFACTORY_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/RuntimeConfig.h"
#include "joynr/Directory.h"
#include "joynr/IMessagingStubFactory.h"

namespace joynr {

class IMessaging;

namespace system { class Address; }
class InProcessMessagingSkeleton;
class ICommunicationManager;

/**
  * Creates/Stores <Middleware>MessagingStubs. MessagingStubs are used to contact remote ClusterControllers (HttpCommunicationManager)
  * and libjoynrs (dummy<Libjoynr>Skeleton) on the machine.
  * A libjoynr does not need a MessagingStubFactory, as each libJoynr has one MessagingStub that connects it to its cc,
  * and will nevere use any other MessagingStubs.
  *
  */

class MessagingStubFactory : public IMessagingStubFactory {

public:
    virtual ~MessagingStubFactory();
    //MessagingStubFactory is created without the necessary skeletons.
    //Those Skeletons must be registered before the MessagingStubFactory is used.
    MessagingStubFactory();

    //void registerInProcessMessagingSkeleton(QSharedPointer<InProcessMessagingSkeleton> messagingSkeleton);

    QSharedPointer<IMessaging> create(
            QString destParticipantId,
            QSharedPointer<joynr::system::Address> destinationAddress);
    void remove(QString destParticipantId);
    bool contains(QString destParticipantId);
    void setCommunicationManager(QSharedPointer<ICommunicationManager> comMgr);
private:
    DISALLOW_COPY_AND_ASSIGN(MessagingStubFactory);
    bool isInProcessMessaging(QSharedPointer<joynr::system::Address> destinationAddress);
    bool isJoynr(QSharedPointer<joynr::system::Address> destinationAddress);
    bool isLocal(QString destParticipantId);
    bool isDbus(QSharedPointer<joynr::system::Address> destinationAddress);
    Directory<QString, IMessaging> partId2MessagingStubDirectory;
    QSharedPointer<ICommunicationManager> communicationManager;

#ifdef USE_DBUS_COMMONAPI_COMMUNICATION
    Directory<QString, IMessaging> dbusMessagingStubDirectory;
#endif // USE_DBUS_COMMONAPI_COMMUNICATION
};


} // namespace joynr
#endif //MESSAGINGSTUBFACTORY
