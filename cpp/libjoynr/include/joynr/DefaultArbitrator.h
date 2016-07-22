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
#ifndef DEFAULTARBITRATOR_H
#define DEFAULTARBITRATOR_H
#include "joynr/PrivateCopyAssign.h"

#include "joynr/Logger.h"
#include <vector>
#include "joynr/ProviderArbitrator.h"
#include <string>
#include "joynr/types/DiscoveryEntry.h"

namespace joynr
{

namespace system
{
class IDiscoverySync;
class DiscoveryEntry;
} // namespace system

class DefaultArbitrator : public ProviderArbitrator
{
public:
    DefaultArbitrator(const std::string& domain,
                      const std::string& interfaceName,
                      const joynr::types::Version& interfaceVersion,
                      joynr::system::IDiscoverySync& discoveryProxy,
                      const DiscoveryQos& discoveryQos);
    void attemptArbitration() override;

private:
    DISALLOW_COPY_AND_ASSIGN(DefaultArbitrator);
    virtual void receiveCapabilitiesLookupResults(
            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries);
    ADD_LOGGER(DefaultArbitrator);
};

} // namespace joynr
#endif // DEFAULTARBITRATOR_H
