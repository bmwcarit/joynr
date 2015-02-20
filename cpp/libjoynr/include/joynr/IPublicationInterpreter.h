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
#ifndef IPUBLICATIONINTERPRETER_H
#define IPUBLICATIONINTERPRETER_H

#include <QSharedPointer>

namespace joynr
{

class ISubscriptionCallback;
class SubscriptionPublication;

/*
  * TODO
  */

class IPublicationInterpreter
{
public:
    virtual ~IPublicationInterpreter()
    {
    }
    // The ReplyCaller is the instance that calls the actual receiver of the reply.
    virtual void execute(QSharedPointer<ISubscriptionCallback> callback,
                         const SubscriptionPublication& subscriptionPublication) = 0;
};

} // namespace joynr
#endif // IPUBLICATIONINTERPRETER_H
