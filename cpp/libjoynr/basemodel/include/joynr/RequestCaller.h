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
    explicit RequestCaller(const std::string& interfaceName, const types::Version& providerVersion);
    explicit RequestCaller(std::string&& interfaceName, types::Version&& providerVersion);

    virtual ~RequestCaller() = default;

    const std::string& getInterfaceName() const;

    // Get and set the attribute listeners listening on the provider
    virtual void registerAttributeListener(
            const std::string& attributeName,
            std::shared_ptr<SubscriptionAttributeListener> attributeListener);
    virtual void unregisterAttributeListener(
            const std::string& attributeName,
            std::shared_ptr<SubscriptionAttributeListener> attributeListener);

    // Get and set the broadcast listeners listening on the provider
    virtual void registerBroadcastListener(
            const std::string& broadcastName,
            std::shared_ptr<UnicastBroadcastListener> broadcastListener);
    virtual void unregisterBroadcastListener(
            const std::string& broadcastName,
            std::shared_ptr<UnicastBroadcastListener> broadcastListener);

    types::Version getProviderVersion();

protected:
    virtual std::shared_ptr<IJoynrProvider> getProvider() = 0;

private:
    DISALLOW_COPY_AND_ASSIGN(RequestCaller);
    std::string _interfaceName;
    types::Version _providerVersion;
};

} // namespace joynr
#endif // REQUESTCALLER_H
