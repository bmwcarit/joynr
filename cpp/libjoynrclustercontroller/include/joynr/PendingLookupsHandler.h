/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

#include <unordered_map>
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/InterfaceAddress.h"
#include "joynr/types/DiscoveryQos.h"

namespace joynr
{

class PendingLookupsHandler
{

public:
    PendingLookupsHandler();
    ~PendingLookupsHandler()
    {
    }

    /*
     * returns true if lookup calls with discovery scope LOCAL_THEN_GLOBAL are ongoing
     */
    bool hasPendingLookups();

    void registerPendingLookup(const std::vector<InterfaceAddress>& interfaceAddresses,
                               const std::shared_ptr<ILocalCapabilitiesCallback>& callback);
    bool isCallbackCalled(const std::vector<InterfaceAddress>& interfaceAddresses,
                          const std::shared_ptr<ILocalCapabilitiesCallback>& callback,
                          const joynr::types::DiscoveryQos& discoveryQos);
    void callbackCalled(const std::vector<InterfaceAddress>& interfaceAddresses,
                        const std::shared_ptr<ILocalCapabilitiesCallback>& callback);
    void callPendingLookups(const InterfaceAddress& interfaceAddress,
                            std::vector<types::DiscoveryEntry> localCapabilities);

private:
    std::unordered_map<InterfaceAddress, std::vector<std::shared_ptr<ILocalCapabilitiesCallback>>>
            _pendingLookups;
};
}
