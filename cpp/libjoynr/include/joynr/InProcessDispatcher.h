/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <string>
#include <memory>

#include "joynr/IDispatcher.h"
#include "joynr/IRequestCallerDirectory.h"
#include "joynr/InProcessAddress.h"
#include "joynr/LibJoynrDirectories.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

namespace boost
{
namespace asio
{
class io_service;
} // namespace asio
} // namespace boost

namespace joynr
{

class IReplyCaller;
class MessagingQos;

class JOYNR_EXPORT InProcessDispatcher : public IDispatcher, public IRequestCallerDirectory
{
public:
    explicit InProcessDispatcher(boost::asio::io_service& ioService);
    ~InProcessDispatcher() override;

    void addReplyCaller(const std::string& requestReplyId,
                        std::shared_ptr<IReplyCaller> replyCaller,
                        const MessagingQos& qosSettings) override;

    void removeReplyCaller(const std::string& requestReplyId) override;

    void addRequestCaller(const std::string& participantId,
                          std::shared_ptr<RequestCaller> requestCaller) override;

    void removeRequestCaller(const std::string& participantId) override;

    void receive(const JoynrMessage& message) override;

    void registerSubscriptionManager(
            std::shared_ptr<ISubscriptionManager> subscriptionManager) override;

    void registerPublicationManager(PublicationManager* publicationManager) override;

    std::shared_ptr<RequestCaller> lookupRequestCaller(const std::string& participantId) override;

    bool containsRequestCaller(const std::string& participantId) override;

private:
    DISALLOW_COPY_AND_ASSIGN(InProcessDispatcher);
    RequestCallerDirectory requestCallerDirectory;
    ReplyCallerDirectory replyCallerDirectory;
    PublicationManager* publicationManager;
    std::shared_ptr<ISubscriptionManager> subscriptionManager;
    ADD_LOGGER(InProcessDispatcher);
};

} // namespace joynr
#endif // INPROCESSDISPATCHER_H
