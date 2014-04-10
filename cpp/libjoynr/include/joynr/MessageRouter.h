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
#ifndef MESSAGEROUTER_H
#define MESSAGEROUTER_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/ObjectWithDecayTime.h"
#include "joynr/JoynrMessage.h"
#include "joynr/MessagingQos.h"
#include "joynr/IMessaging.h"
#include "joynr/MessagingSettings.h"
#include "joynr/system/RoutingProxy.h"
#include "joynr/system/RoutingProvider.h"
#include "joynr/RequestStatus.h"
#include "joynr/ICallback.h"

#include <QSharedPointer>
#include <QDateTime>
#include <QThreadPool>
#include <QSemaphore>
#include <QPair>
#include <QMap>
#include <QSet>
#include <QMutex>

namespace joynr {


class IMessagingStubFactory;
class JoynrMessagingEndpointAddress;
namespace joynr_logging { class Logger; }
class DelayedScheduler;
class ThreadPoolDelayedScheduler;
namespace system { class Address; }
template<typename Key, typename T> class Directory;
class ICommunicationManager;

/**
  * Class MessageRouter receives incoming JoynrMessages on the ClusterController
  * and forwards them either to a remote ClusterController or to a LibJoynr on the machine.
  *
  *  1 extracts the destination participant ID and looks up the EndpointAddress in the MessagingEndpointDirectory
  *  2 creates a <Middleware>MessagingStub by calling MessagingStubFactory.create(EndpointAddress addr)
  *  3 forwards the message using the <Middleware>MessagingStub.send(JoynrMessage msg)
  *
  *  In sending, a ThreadPool of default size 6 is used with a 500ms default retry interval.
  */

class JOYNR_EXPORT MessageRouter : public joynr::system::RoutingProvider {
public:
    MessageRouter(
            Directory<QString, joynr::system::Address>* routingTable,
            IMessagingStubFactory* messagingStubFactory,
            int messageSendRetryInterval = 500,
            int maxThreads = 6
    );

    MessageRouter(
            Directory<QString, joynr::system::Address>* routingTable,
            IMessagingStubFactory* messagingStubFactory,
            QSharedPointer<joynr::system::Address> incomingAddress,
            int messageSendRetryInterval = 500,
            int maxThreads = 6
    );

    virtual ~MessageRouter();

    /**
     * @brief Forwards the message to its endpoint (determined by inspecting the message
     * header). NOTE: the init method must be called before the first message is routed.
     *
     * @param message the message to route.
     * @param qos the QoS used to route the message.
     */
    virtual void route(const JoynrMessage& message, const MessagingQos& qos);

    // inherited method from joynr::system::RoutingProvider
    virtual void addNextHop(
            joynr::RequestStatus& joynrInternalStatus,
            QString participantId,
            joynr::system::ChannelAddress channelAddress
    );
    // inherited method from joynr::system::RoutingProvider
    virtual void addNextHop(
            joynr::RequestStatus& joynrInternalStatus,
            QString participantId,
            joynr::system::CommonApiDbusAddress commonApiDbusAddress
    );
    // inherited method from joynr::system::RoutingProvider
    virtual void addNextHop(
            joynr::RequestStatus& joynrInternalStatus,
            QString participantId,
            joynr::system::BrowserAddress browserAddress
    );
    // inherited method from joynr::system::RoutingProvider
    virtual void addNextHop(
            joynr::RequestStatus& joynrInternalStatus,
            QString participantId,
            joynr::system::WebSocketAddress webSocketAddress
    );
    // inherited method from joynr::system::RoutingProvider
    virtual void removeNextHop(
            joynr::RequestStatus& joynrInternalStatus,
            QString participantId
    );
    // inherited method from joynr::system::RoutingProvider
    virtual void resolveNextHop(
            joynr::RequestStatus& joynrInternalStatus,
            bool& resolved,
            QString participantId
    );

    void addProvisionedNextHop(QString participantId, QSharedPointer<joynr::system::Address> address);

    void setParentRouter(joynr::system::RoutingProxy* parentRouter, QSharedPointer<joynr::system::Address> parentAddress, QString parentParticipantId);

    virtual void addNextHop(
            QString participantId,
            QSharedPointer<joynr::system::Address> inprocessAddress
    );

    friend class MessageRunnable;
    friend class ResolveCallBack;

private:
    DISALLOW_COPY_AND_ASSIGN(MessageRouter);
    IMessagingStubFactory* messagingStubFactory;
    Directory<QString, joynr::system::Address>* routingTable;
    QThreadPool threadPool;
    ThreadPoolDelayedScheduler* delayedScheduler;
    joynr::system::RoutingProxy* parentRouter;
    QSharedPointer<joynr::system::Address> parentAddress;
    QSharedPointer<joynr::system::Address> incomingAddress;
    static joynr_logging::Logger* logger;

    QMap<QString, QPair<JoynrMessage, MessagingQos>>* messageQueue;
    mutable QMutex messageQueueMutex;
    QSet<QString>* runningParentResolves;
    mutable QMutex parentResolveMutex;

    void init(int messageSendRetryInterval, int maxThreads);
    void addNextHopToParent(joynr::RequestStatus& joynrInternalStatus, QString participantId);

    void sendMessage(const JoynrMessage& message,
                     const MessagingQos& qos,
                     QSharedPointer<joynr::system::Address> destAddress);

    void sendMessageToParticipant(QString& destinationPartId);

    bool isChildMessageRouter();
};

/**
 * Class to handle callbacks from resolve requests to the parent message router.
 */

class ResolveCallBack: public joynr::ICallback<bool> {
public:
    ResolveCallBack(MessageRouter& messageRouter, QString destinationParticipantId);

    void onFailure(const RequestStatus status);

    void onSuccess(const RequestStatus status, bool resolved);

private:
    MessageRouter& messageRouter;
    QString destinationPartId;
    static joynr_logging::Logger* logger;
};

/**
 * Class to send message
 */
class MessageRunnable: public QRunnable, public ObjectWithDecayTime {
public:
    MessageRunnable(const JoynrMessage& message,
                    const MessagingQos& qos,
                    QSharedPointer<IMessaging> messagingStub);
    void run();

private:
    JoynrMessage message;
    MessagingQos qos;
    QSharedPointer<IMessaging> messagingStub;
    static joynr_logging::Logger* logger;
};

} // namespace joynr
#endif //MESSAGEROUTER_H
