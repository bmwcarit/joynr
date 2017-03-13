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
#ifndef PROVIDERARBITRATORFACTORY_H
#define PROVIDERARBITRATORFACTORY_H

#include <string>
#include <memory>

#include "joynr/JoynrExport.h"
#include "joynr/Arbitrator.h"

namespace joynr
{

namespace system
{
class IDiscoverySync;
} // namespace system

class JOYNR_EXPORT ArbitratorFactory
{

public:
    /*
     *  Creates an arbitrator object using the type specified in the qosParameters.
     */
    static std::unique_ptr<Arbitrator> createArbitrator(
            const std::string& domain,
            const std::string& interfaceName,
            const types::Version& interfaceVersion,
            joynr::system::IDiscoverySync& discoveryProxy,
            const DiscoveryQos& discoveryQos);
};

} // namespace joynr
#endif // PROVIDERARBITRATORFACTORY_H
