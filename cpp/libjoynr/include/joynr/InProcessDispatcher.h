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
#ifndef INPROCESSDISPATCHER_H
#define INPROCESSDISPATCHER_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrExport.h"
#include "joynr/IDispatcher.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/InProcessAddress.h"
#include "joynr/LibJoynrDirectories.h"
#include "joynr/joynrlogging.h"

#include <QString>
#include <QSharedPointer>

namespace joynr {

class MessagingQos;

class JOYNR_EXPORT InProcessDispatcher : public IDispatcher , public IRequestCallerDirectory {
public:
    InProcessDispatcher();
    virtual ~InProcessDispatcher ();

    virtual void addReplyCaller(const QString& requestReplyId,
                                QSharedPointer<IReplyCaller> replyCaller,
                                const MessagingQos& qosSettings);

    virtual void removeReplyCaller(const QString& requestReplyId);

    virtual void addRequestCaller(const QString& participantId,
                                  QSharedPointer<RequestCaller> requestCaller);

    virtual void removeRequestCaller(const QString& participantId);

    virtual void receive(const JoynrMessage& message);

    virtual void registerSubscriptionManager(SubscriptionManager* subscriptionManager);

    virtual void registerPublicationManager(PublicationManager* publicationManager);

    QSharedPointer<RequestCaller> lookupRequestCaller(const QString& participantId);

    bool containsRequestCaller(const QString& participantId);

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessDispatcher);
    RequestCallerDirectory requestCallerDirectory;
    ReplyCallerDirectory replyCallerDirectory;
    PublicationManager* publicationManager;
    SubscriptionManager* subscriptionManager;
    static joynr_logging::Logger* logger;
};


} // namespace joynr
#endif //INPROCESSDISPATCHER_H
