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

#ifndef ARBITRATOR_H
#define ARBITRATOR_H

#include <atomic>
#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include <boost/variant.hpp>

#include "joynr/ArbitrationStrategyFunction.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/Future.h"
#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/types/DiscoveryEntryWithMetaInfo.h"
#include "joynr/types/DiscoveryQos.h"
#include "joynr/types/Version.h"

namespace joynr
{

namespace system
{
class IDiscoveryAsync;
} // namespace system

/*
 *  Base class for different arbitration strategies.
 */
class JOYNR_EXPORT Arbitrator : public std::enable_shared_from_this<Arbitrator>
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
               std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy,
               const DiscoveryQos& discoveryQos,
               std::unique_ptr<const ArbitrationStrategyFunction> arbitrationStrategyFunction);

    /*
     *  Arbitrate until successful or until a timeout occurs
     */
    void startArbitration(
            std::function<void(const joynr::types::DiscoveryEntryWithMetaInfo& discoveryEntry)>
                    onSuccess,
            std::function<void(const exceptions::DiscoveryException& exception)> onError);

    void stopArbitration();

private:
    /*
     *  attemptArbitration() has to be implemented by the concrete arbitration strategy.
     *  This method attempts arbitration and sets arbitrationStatus to indicate the
     *  state of arbitration.
     */
    virtual void attemptArbitration();

    virtual void receiveCapabilitiesLookupResults(
            const std::vector<joynr::types::DiscoveryEntryWithMetaInfo>& discoveryEntries);

    std::int64_t getDurationMs() const;

    /*
     * Check that the pending future has completed successfully and reset it.
     */
    void validatePendingFuture();

    void assertNoPendingFuture();

    std::mutex pendingFutureMutex;
    boost::variant<
            std::shared_ptr<joynr::Future<joynr::types::DiscoveryEntryWithMetaInfo>>,
            std::shared_ptr<joynr::Future<std::vector<joynr::types::DiscoveryEntryWithMetaInfo>>>>
            pendingFuture;

    std::weak_ptr<joynr::system::IDiscoveryAsync> discoveryProxy;
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
    Semaphore semaphore;
    bool arbitrationFinished;
    std::atomic<bool> arbitrationRunning;
    std::atomic<bool> keepArbitrationRunning;
    std::thread arbitrationThread;
    std::chrono::steady_clock::time_point startTimePoint;
    std::once_flag _onceFlag;
    ADD_LOGGER(Arbitrator)
};

} // namespace joynr
#endif // ARBITRATOR_H
