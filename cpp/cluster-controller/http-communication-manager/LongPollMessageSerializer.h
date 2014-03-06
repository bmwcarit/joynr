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
#ifndef LONGPOLLMESSAGESERIALIZER_H
#define LONGPOLLMESSAGESERIALIZER_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include "cluster-controller/http-communication-manager/IMessageReceiver.h"

#include <QString>

class QByteArray;

namespace joynr {

namespace system { class Address; }
template<typename Key, typename T> class Directory;
class MessageRouter;
namespace joynr_logging { class Logger; }

/**
  * Is used by the LongpollMessageReceiver to forward messages to the MessageRouter.
  *
  */

class JOYNRCLUSTERCONTROLLER_EXPORT LongPollMessageSerializer : public IMessageReceiver {
public:
    LongPollMessageSerializer(MessageRouter* messageRouter, Directory<QString, joynr::system::Address>* partId2EndpointAddrDirectory);
    virtual ~LongPollMessageSerializer();
    void serializedMessageReceived(const QByteArray& serializedMessage);

private:
    DISALLOW_COPY_AND_ASSIGN(LongPollMessageSerializer);
    MessageRouter* messageRouter;
    Directory<QString, joynr::system::Address>* partId2EndpointAddrDirectory;
    static joynr_logging::Logger* logger;
};


} // namespace joynr
#endif //LONGPOLLMESSAGESERIALIZER_H
