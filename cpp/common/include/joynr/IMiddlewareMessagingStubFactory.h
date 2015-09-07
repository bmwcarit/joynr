/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
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
#ifndef IMIDDLEWAREMESSAGINGSTUBFACTORY_H
#define IMIDDLEWAREMESSAGINGSTUBFACTORY_H

#include <QString>
#include <QSharedPointer>

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

class IMiddlewareMessagingStubFactory
{
public:
    virtual ~IMiddlewareMessagingStubFactory()
    {
    }
    virtual QSharedPointer<IMessaging> create(
            const joynr::system::routingtypes::QtAddress& destAddress) = 0;
    virtual bool canCreate(const joynr::system::routingtypes::QtAddress& destAddress) = 0;
};

} // namespace joynr

#endif // IMIDDLEWAREMESSAGINGSTUBFACTORY_H
