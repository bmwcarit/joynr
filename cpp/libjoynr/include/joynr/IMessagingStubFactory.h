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
#ifndef IMESSAGINGSTUBFACTORY_H
#define IMESSAGINGSTUBFACTORY_H

#include <string>

namespace joynr
{

namespace system
{

namespace routingtypes
{
class QtAddress;
}
}
class IMessaging;

class IMessagingStubFactory
{
public:
    virtual ~IMessagingStubFactory()
    {
    }
    virtual QSharedPointer<IMessaging> create(
            std::string destParticipantId,
            const joynr::system::routingtypes::QtAddress& destEndpointAddress) = 0;
    virtual void remove(std::string destParticipantId) = 0;
    virtual bool contains(std::string destParticipantId) = 0;
};

} // namespace joynr
#endif // IMESSAGINGSTUBFACTORY_H
