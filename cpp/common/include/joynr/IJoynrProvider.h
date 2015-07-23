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
#ifndef IJOYNRPROVIDER_H
#define IJOYNRPROVIDER_H

#include <string>
#include <QSharedPointer>

namespace joynr
{
class IAttributeListener;
class IBroadcastListener;
class IBroadcastFilter;

namespace types
{
class StdProviderQos;
}

class IJoynrProvider
{
public:
    virtual ~IJoynrProvider()
    {
    }

    virtual types::StdProviderQos getProviderQos() const = 0;

    /**
     * Register an object that will be informed when the value of an attribute changes
     */
    virtual void registerAttributeListener(const std::string& attributeName,
                                           IAttributeListener* attributeListener) = 0;
    /**
     * Unregister and delete an attribute listener
     */
    virtual void unregisterAttributeListener(const std::string& attributeName,
                                             IAttributeListener* attributeListener) = 0;

    /**
     * Register an object that will be informed when an event occurs
     */
    virtual void registerBroadcastListener(const std::string& broadcastName,
                                           IBroadcastListener* broadcastListener) = 0;

    /**
     * Unregister and delete a broadcast listener
     */
    virtual void unregisterBroadcastListener(const std::string& broadcastName,
                                             IBroadcastListener* broadcastListener) = 0;

    virtual void addBroadcastFilter(QSharedPointer<IBroadcastFilter> filter) = 0;

    virtual std::string getInterfaceName() const = 0;
};

} // namespace joynr
#endif // IJOYNRPROVIDER_H
