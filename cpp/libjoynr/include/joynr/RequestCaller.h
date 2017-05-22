/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef REQUESTCALLER_H
#define REQUESTCALLER_H

#include <string>

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/types/Version.h"

namespace joynr
{

class SubscriptionAttributeListener;
class UnicastBroadcastListener;
class IJoynrProvider;

class JOYNR_EXPORT RequestCaller
{
public:
    explicit RequestCaller(const std::string& interfaceName);
    explicit RequestCaller(std::string&& interfaceName);

    virtual ~RequestCaller() = default;

    const std::string& getInterfaceName() const;

    // Get and set the attribute listeners listening on the provider
    virtual void registerAttributeListener(const std::string& attributeName,
                                           SubscriptionAttributeListener* attributeListener) = 0;
    virtual void unregisterAttributeListener(const std::string& attributeName,
                                             SubscriptionAttributeListener* attributeListener) = 0;

    // Get and set the broadcast listeners listening on the provider
    virtual void registerBroadcastListener(const std::string& broadcastName,
                                           UnicastBroadcastListener* broadcastListener) = 0;
    virtual void unregisterBroadcastListener(const std::string& broadcastName,
                                             UnicastBroadcastListener* broadcastListener) = 0;

    virtual types::Version getProviderVersion() = 0;

protected:
    virtual std::shared_ptr<IJoynrProvider> getProvider() = 0;

private:
    DISALLOW_COPY_AND_ASSIGN(RequestCaller);
    std::string interfaceName;
};

} // namespace joynr
#endif // REQUESTCALLER_H
