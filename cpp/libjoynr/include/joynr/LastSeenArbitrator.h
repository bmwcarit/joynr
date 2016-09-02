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
#ifndef LASTSEENARBITRATOR_H
#define LASTSEENARBITRATOR_H
#include <string>
#include <vector>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/Arbitrator.h"

namespace joynr
{

namespace system
{
class IDiscoverySync;
} // namespace system
namespace types
{
class DiscoveryEntry;
class Version;
class DiscoveryQos;
} // namespace types

/*
  * The LastSeenArbitrator arbitrates according to the LastSeenDateMs of the provider.
  * It arbitrates to the provider with the latest LastSeenDateMs.
  * This is the default arbitration strategy.
  */
class JOYNR_EXPORT LastSeenArbitrator : public Arbitrator
{

public:
    ~LastSeenArbitrator() override = default;
    LastSeenArbitrator(const std::string& domain,
                       const std::string& interfaceName,
                       const joynr::types::Version& interfaceVersion,
                       joynr::system::IDiscoverySync& discoveryProxy,
                       const DiscoveryQos& discoveryQos);

protected:
    std::string filterDiscoveryEntries(
            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries) override;

private:
    void receiveCapabilitiesLookupResults(
            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries);

    DISALLOW_COPY_AND_ASSIGN(LastSeenArbitrator);
    ADD_LOGGER(LastSeenArbitrator);
};

} // namespace joynr
#endif // LASTSEENARBITRATOR_H
