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
#ifndef QOSARBITRATOR_H
#define QOSARBITRATOR_H
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"

#include <vector>
#include "joynr/Arbitrator.h"
#include <string>

namespace joynr
{

namespace system
{
class IDiscoverySync;
} // namespace system
namespace types
{
class DiscoveryEntry;
} // namespace types

/*
  * The QoS Arbitrator arbitrates according to the QoS of the provider.
  * Currently it arbitrates to the provider with the highest priority.
  */
class JOYNR_EXPORT QosArbitrator : public Arbitrator
{

public:
    ~QosArbitrator() override = default;
    QosArbitrator(const std::string& domain,
                  const std::string& interfaceName,
                  const joynr::types::Version& interfaceVersion,
                  joynr::system::IDiscoverySync& discoveryProxy,
                  const DiscoveryQos& discoveryQos);

protected:
    std::string filterDiscoveryEntries(
            const std::vector<joynr::types::DiscoveryEntry>& discoveryEntries) override;

private:
    DISALLOW_COPY_AND_ASSIGN(QosArbitrator);
    static int ARBITRATION_RETRY_INTERVAL;
    ADD_LOGGER(QosArbitrator);
};

} // namespace joynr
#endif // QOSARBITRATOR_H
