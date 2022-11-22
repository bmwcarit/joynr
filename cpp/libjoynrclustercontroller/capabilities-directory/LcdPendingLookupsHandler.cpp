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
#include "joynr/LcdPendingLookupsHandler.h"
#include "joynr/ILocalCapabilitiesCallback.h"
#include "joynr/LCDUtil.h"
#include "joynr/types/DiscoveryQos.h"

namespace joynr
{

LcdPendingLookupsHandler::LcdPendingLookupsHandler() : _pendingLookups()
{
}

void LcdPendingLookupsHandler::callPendingLookups(
        const InterfaceAddress& interfaceAddress,
        std::vector<types::DiscoveryEntry> localCapabilities)
{
    if (_pendingLookups.find(interfaceAddress) == _pendingLookups.cend()) {
        return;
    }
    if (localCapabilities.empty()) {
        return;
    }
    auto localCapabilitiesWithMetaInfo = LCDUtil::convert(true, localCapabilities);

    std::vector<std::shared_ptr<ILocalCapabilitiesCallback>> callbacksToRemove;
    for (const std::shared_ptr<ILocalCapabilitiesCallback>& callback :
         _pendingLookups[interfaceAddress]) {
        callbacksToRemove.push_back(callback);
        callback->capabilitiesReceived(localCapabilitiesWithMetaInfo);
    }

    // Note that a pending lookup might have been done for multiple damains
    // in parallel resulting in multiple entries using the same callback.
    // Identify all callbacks associated with a lookup for the interfaceAddress
    // of the provider about to be registered, then also remove all other entries
    // associated with the same callbacks since the search criteria is fulfilled
    // by registering a provider that matches any of the domains searched for.
    std::vector<InterfaceAddress> allInterfaces;
    for (const auto& elem : _pendingLookups) {
        allInterfaces.push_back(elem.first);
    }
    for (const auto& callback : callbacksToRemove) {
        callbackCalled(allInterfaces, callback);
    }
}

void LcdPendingLookupsHandler::registerPendingLookup(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        _pendingLookups[address].push_back(callback); // if no entry exists for key address, an
                                                      // empty list is automatically created
    }
}

bool LcdPendingLookupsHandler::hasPendingLookups()
{
    return !_pendingLookups.empty();
}

bool LcdPendingLookupsHandler::isCallbackCalled(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback,
        const joynr::types::DiscoveryQos& discoveryQos)
{
    // only if discovery scope is joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL, the
    // callback can potentially already be called, as a matching capability has been added
    // to the local capabilities directory while waiting for capabilitiesclient->lookup result
    if (discoveryQos.getDiscoveryScope() != joynr::types::DiscoveryScope::LOCAL_THEN_GLOBAL) {
        return false;
    }
    for (const InterfaceAddress& address : interfaceAddresses) {
        if (_pendingLookups.find(address) == _pendingLookups.cend()) {
            return true;
        }
        if (std::find(_pendingLookups[address].cbegin(),
                      _pendingLookups[address].cend(),
                      callback) == _pendingLookups[address].cend()) {
            return true;
        }
    }
    return false;
}

void LcdPendingLookupsHandler::callbackCalled(
        const std::vector<InterfaceAddress>& interfaceAddresses,
        const std::shared_ptr<ILocalCapabilitiesCallback>& callback)
{
    for (const InterfaceAddress& address : interfaceAddresses) {
        if (_pendingLookups.find(address) != _pendingLookups.cend()) {
            std::vector<std::shared_ptr<ILocalCapabilitiesCallback>>& callbacks =
                    _pendingLookups[address];
            util::removeAll(callbacks, callback);
            if (_pendingLookups[address].empty()) {
                _pendingLookups.erase(address);
            }
        }
    }
}
} // namespace joynr
