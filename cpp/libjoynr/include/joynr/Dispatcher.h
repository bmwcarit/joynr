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
/*
 * Dispatcher.h
 *
 *  Created on: Aug 9, 2011
 *      Author: grape
 */

#ifndef DISPATCHER_H_
#define DISPATCHER_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include <QString>
#include <QSharedPointer>
#include "joynr/IDispatcher.h"
#include "joynr/LibJoynrDirectories.h"
#include "joynr/joynrlogging.h"

namespace joynr {

class IReplyCaller;
class MessagingQos;
class RequestCaller;
class JoynrMessage;
class JoynrMessageSender;

class JOYNR_EXPORT Dispatcher : public IDispatcher {

public:
    //ownership of messageSender is not passed to dispatcher, so dispatcher is not responsible for deleting it.
    //Todo: should be changed to QSP or reference.
    explicit Dispatcher(JoynrMessageSender* messageSender, int maxThreads = 4);

    virtual ~Dispatcher() ;

    virtual void addReplyCaller(const QString& requestReplyId,
                                QSharedPointer<IReplyCaller> replyCaller,
                                const MessagingQos& qosSettings);

    virtual void removeReplyCaller(const QString& requestReplyId);

    virtual void addRequestCaller(const QString& participantId,
                                  QSharedPointer<RequestCaller> requestCaller);

    virtual void removeRequestCaller(const QString& participantId);

    virtual void receive(const JoynrMessage& message, const MessagingQos& qos);

    virtual void registerSubscriptionManager(SubscriptionManager* subscriptionManager);

    virtual void registerPublicationManager(PublicationManager* publicationManager);

private:
    void handleRequestReceived(const JoynrMessage& message, const MessagingQos& qos);
    void handleReplyReceived(const JoynrMessage& message);
    void handlePublicationReceived(const JoynrMessage& message);
    void handleSubscriptionRequestReceived(const JoynrMessage& message);
    void handleSubscriptionStopReceived(const JoynrMessage& message);

private:
    DISALLOW_COPY_AND_ASSIGN(Dispatcher);
    JoynrMessageSender* messageSender;
    RequestCallerDirectory requestCallerDirectory;
    ReplyCallerDirectory replyCallerDirectory;
    PublicationManager* publicationManager;
    SubscriptionManager* subscriptionManager;
    QThreadPool handleReceivedMessageThreadPool;
    static joynr_logging::Logger* logger;
    QMutex subscriptionHandlingMutex;


    friend class ReceivedMessageRunnable;
};

} // namespace joynr
#endif /* DISPATCHER_H_ */
