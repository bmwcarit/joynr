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

#ifndef PERFORMANCECONSUMER_H
#define PERFORMANCECONSUMER_H

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <limits>
#include <numeric>
#include <exception>
#include <random>

#include "../common/PerformanceTest.h"
#include "joynr/JoynrRuntime.h"
#include "joynr/DiscoveryQos.h"
#include "joynr/infrastructure/GlobalCapabilitiesDirectoryProxy.h"
#include "joynr/ProxyBuilder.h"
#include "joynr/Semaphore.h"

namespace joynr
{

struct IPerformanceConsumer
{
    virtual void runLookup(const std::string& participantId) = 0;
    virtual void runLookup(const std::vector<std::string>& domains,
                           const std::string& interfaceName) = 0;
};

template <typename Impl>
class PerformanceConsumer : public IPerformanceConsumer
{
public:
    using Clock = std::chrono::high_resolution_clock;
    using ClockResolution = std::chrono::microseconds;
    using GCDProxy = infrastructure::GlobalCapabilitiesDirectoryProxy;

public:
    PerformanceConsumer(std::shared_ptr<JoynrRuntime> joynrRuntime,
                        std::size_t calls,
                        const std::string& domain,
                        std::size_t maxInflightCalls,
                        std::size_t repetition,
                        const std::string& containerId,
                        const std::vector<std::string>& gbids)
            : _runtime(std::move(joynrRuntime)),
              _calls(calls),
              _domain(domain),
              _maxInflightCalls(maxInflightCalls),
              _repetition(repetition),
              _containerId(containerId),
              _gbids(std::move(gbids)),
              _durationVector(_calls)
    {
        if (_calls == 0) {
            throw std::invalid_argument("_calls must be >= 1");
        }

        // Build a proxy builder
        std::shared_ptr<ProxyBuilder<GCDProxy>> proxyBuilder =
                _runtime->createProxyBuilder<GCDProxy>(_domain);

        if (!proxyBuilder) {
            std::cerr << "Failed to create GCD proxy builder" << std::endl;
            exit(EXIT_FAILURE);
        }

        // Find the provider with the highest priority set in ProviderQos
        DiscoveryQos discoveryQos;
        discoveryQos.setDiscoveryTimeoutMs(240000); // 4 Min
        discoveryQos.setCacheMaxAgeMs(std::numeric_limits<std::int64_t>::max());
        discoveryQos.setArbitrationStrategy(DiscoveryQos::ArbitrationStrategy::HIGHEST_PRIORITY);
        discoveryQos.setRetryIntervalMs(5000);            // 5 seconds
        MessagingQos messagingQos = MessagingQos(240000); // 4 Minutes

        // Build a proxy
        try {
            std::cout << "Attempting to build GCD proxy: \n";
            _proxy = proxyBuilder->setMessagingQos(messagingQos)
                             ->setDiscoveryQos(discoveryQos)
                             ->setGbids(_gbids)
                             ->build();
        } catch (const std::exception& e) {
            std::cerr << "Failed to build GCD proxy: " << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "Building GCD proxy returns successful: \n";
    }

    void runLookup(const std::string& particpantId) override
    {
        run(&Impl::callLookupByParticipantId, particpantId);
    }

    void runLookup(const std::vector<std::string>& domains,
                   const std::string& interfaceName) override
    {
        run(&Impl::callLookupByDomainsInterfaceName, domains, interfaceName);
    }

    template <typename LoopFun, typename... Args>
    void run(LoopFun fun, Args&&... args)
    {
        const auto startLoop = Clock::now();
        (static_cast<Impl*>(this)->*fun)(std::forward<Args>(args)...);
        const auto endLoop = Clock::now();
        auto totalDuration = std::chrono::duration_cast<ClockResolution>(endLoop - startLoop);

        PerformanceTest::printStatistics(
                _durationVector, totalDuration, _repetition, _calls, _containerId);
    }

protected:
    std::size_t _calls;
    std::string _domain;
    std::atomic<uint32_t> _maxInflightCalls;
    std::size_t _repetition;
    const std::string _containerId;
    const std::vector<std::string> _gbids;
    std::vector<ClockResolution> _durationVector;
    std::shared_ptr<GCDProxy> _proxy;

private:
    std::shared_ptr<JoynrRuntime> _runtime;
};

class SyncConsumer : public PerformanceConsumer<SyncConsumer>
{
public:
    using PerformanceConsumer::PerformanceConsumer;

    void callLookupByParticipantId(const std::string& participantId)
    {
        joynr::types::GlobalDiscoveryEntry result;
        auto fun = [this, &participantId, &result]() {
            _proxy->lookup(result, participantId, _gbids, MessagingQos(600000));
        };
        loop(fun);
        std::cout << "PT RESULT success, lookup by participantId: CPP SYNC consumer \n";
    }

    void callLookupByDomainsInterfaceName(const std::vector<std::string>& domains,
                                          const std::string& interfaceName)
    {
        std::vector<joynr::types::GlobalDiscoveryEntry> result;
        auto fun = [this, &domains, &interfaceName, &result]() {
            _proxy->lookup(result, domains, interfaceName, _gbids, MessagingQos(600000));
        };
        loop(fun);
        std::cout << "PT RESULT success, lookup by domains and interface: CPP SYNC consumer \n";
    }

private:
    template <typename Fun>
    void loop(Fun&& fun)
    {
        for (std::size_t i = 0; i < _calls; ++i) {
            const auto start = Clock::now();
            fun();
            const auto end = Clock::now();
            _durationVector[i] = std::chrono::duration_cast<ClockResolution>(end - start);
        }
    }
};

class ASyncConsumer : public PerformanceConsumer<ASyncConsumer>
{
public:
    using PerformanceConsumer::PerformanceConsumer;

    void callLookupByParticipantId(const std::string& participantId)
    {
        auto fun = [this, &participantId](auto onSuccess) {
            _proxy->lookupAsync(
                    participantId,
                    _gbids,
                    onSuccess,
                    [](const joynr::types::DiscoveryError::Enum& /*onApplicationError*/) {},
                    [](const joynr::exceptions::JoynrRuntimeException& /*onRuntimeError*/) {},
                    MessagingQos(600000));
        };
        loop(fun);
        std::cout << "PT RESULT success, lookup by participantId: CPP ASYNC consumer \n";
    }

    void callLookupByDomainsInterfaceName(const std::vector<std::string>& domains,
                                          const std::string& interfaceName)
    {
        auto fun = [this, &domains, &interfaceName](auto onSuccess) {
            _proxy->lookupAsync(
                    domains,
                    interfaceName,
                    _gbids,
                    onSuccess,
                    [](const joynr::types::DiscoveryError::Enum& /*onApplicationError*/) {},
                    [](const joynr::exceptions::JoynrRuntimeException& /*onRuntimeError*/) {},
                    MessagingQos(600000));
        };
        loop(fun);
        std::cout << "PT RESULT success, lookup by domains and interface: CPP ASYNC consumer \n";
    }

    void notifyDone()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _done = true;
        lock.unlock();
        _cv.notify_one();
    }

private:
    template <typename Fun>
    void loop(Fun&& fun)
    {
        _done = false;
        _capResultCounterReceived = 0;
        auto semaphore = std::make_shared<joynr::Semaphore>(_maxInflightCalls);
        for (uint32_t i = 0; i < _calls; ++i) {
            semaphore->wait();
            const auto start = Clock::now();
            auto onSuccess = [this, i, start, semaphore](const auto&) {
                const auto end = Clock::now();
                _durationVector[i] = std::chrono::duration_cast<ClockResolution>(end - start);
                _capResultCounterReceived++;
                semaphore->notify();
                // check if this is the results of last run
                if (_capResultCounterReceived == _calls) {
                    this->notifyDone();
                }
            };
            fun(onSuccess);
        }

        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this] { return _done; });
    }

    std::atomic<uint32_t> _capResultCounterReceived;
    bool _done = false;
    std::mutex _mutex;
    std::condition_variable _cv;
};

} // namespace joynr

#endif // PERFORMANCECONSUMER_H
