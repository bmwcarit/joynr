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

#ifndef PROVIDERARBITRATOR_H
#define PROVIDERARBITRATOR_H
#include <string>
#include <unordered_set>
#include <vector>
#include <functional>
#include <atomic>
#include <thread>

#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/JoynrException.h"

namespace joynr
{

namespace system
{
class IDiscoveryAsync;
} // namespace system

/*
 *  Base class for different arbitration strategies.
 */
class JOYNR_EXPORT Arbitrator
{

public:
    virtual ~Arbitrator();

    /*
     *  Creates a new Arbitrator object which blocks the arbitration finished
     *  notification as long as no callback onject has been specified.
     *  This blocking is need for example for the fixed channel arbitrator which
     *  sets the channelId instantly.
     */
    Arbitrator(const std::string& domain,
               const std::string& interfaceName,
               const joynr::types::Version& interfaceVersion,
               joynr::system::IDiscoveryAsync& discoveryProxy,
               const DiscoveryQos& discoveryQos,
               std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction);

    /*
     *  Arbitrate until successful or until a timeout occurs
     */
    void startArbitration(
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& discoveryEntry)>
                    onSuccess,
            std::function<void(const exceptions::DiscoveryException& exception)> onError);

private:
    /*
     *  attemptArbitration() has to be implemented by the concrete arbitration strategy.
     *  This method attempts arbitration and sets arbitrationStatus to indicate the
     *  state of arbitration.
     */
    virtual void attemptArbitration();

    virtual void receiveCapabilitiesLookupResults(
            const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& discoveryEntries);

    joynr::system::IDiscoveryAsync& discoveryProxy;
    DiscoveryQos discoveryQos;
    joynr::types::DiscoveryQos systemDiscoveryQos;
    std::vector<std::string> domains;
    std::string interfaceName;
    joynr::types::Version interfaceVersion;
    std::unordered_set<joynr::types::Version> discoveredIncompatibleVersions;
    exceptions::DiscoveryException arbitrationError;
    std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction;
    std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& discoveryEntry)>
            onSuccessCallback;
    std::function<void(const exceptions::DiscoveryException& exception)> onErrorCallback;

    DISALLOW_COPY_AND_ASSIGN(Arbitrator);
    bool arbitrationFinished;
    std::atomic<bool> arbitrationRunning;
    std::atomic<bool> keepArbitrationRunning;
    std::thread arbitrationThread;
    ADD_LOGGER(Arbitrator);
};

} // namespace joynr
#endif // PROVIDERARBITRATOR_H
