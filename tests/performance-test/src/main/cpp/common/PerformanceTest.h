/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#ifndef PERFORMANCE_TEST_H
#define PERFORMANCE_TEST_H

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <utility>
#include <vector>

using Clock = std::chrono::steady_clock;
using ClockResolution = std::chrono::microseconds;

struct PerformanceTest {
    /**
     * @brief executes a given function @param func with arguments @param args
     * @returns average duration of a function call in milliseconds
     */
    template <typename Function, typename... Args>
    static std::vector<ClockResolution> benchmark(std::uint64_t runs,
                                                  Function&& fun,
                                                  Args&&... args)
    {
        using ResultType = decltype(fun(args...));

        return executeBenchmark(runs,
                                std::forward<Function>(fun),
                                std::forward<Args>(args)...,
                                std::is_void<ResultType>{});
    }

    template <typename Function, typename... Args>
    static std::vector<ClockResolution> executeBenchmark(std::uint64_t runs,
                                                         Function&& fun,
                                                         Args&&... args,
                                                         std::true_type)
    {
        std::vector<ClockResolution> durationVector(runs);
        for (std::size_t i = 0; i < runs; ++i) {
            const auto start = Clock::now();
            fun(std::forward<Args>(args)...);
            const auto end = Clock::now();
            durationVector[i] = std::chrono::duration_cast<ClockResolution>(end - start);
        }
        return durationVector;
    }

    template <typename Function, typename... Args>
    static std::vector<ClockResolution> executeBenchmark(std::uint64_t runs,
                                                         Function&& fun,
                                                         Args&&... args,
                                                         std::false_type)
    {
        std::vector<ClockResolution> durationVector(runs);
        using ResultType = decltype(fun(args...));
        // In order to prevent compiler optimization, the result of the function call
        // is stored in a volatile variable.
        for (std::size_t i = 0; i < runs; ++i) {
            const auto start = Clock::now();
            volatile ResultType result = fun(std::forward<Args>(args)...);
            const auto end = Clock::now();
            durationVector[i] = std::chrono::duration_cast<ClockResolution>(end - start);
        }
        return durationVector;
    }

    static void printStatistics(std::vector<ClockResolution> durationVector,
                                ClockResolution totalDuration)
    {
        using DoubleMilliSeconds = std::chrono::duration<double, std::milli>;
        auto maxDelayDuration = std::chrono::duration_cast<DoubleMilliSeconds>(
                *std::max_element(durationVector.cbegin(), durationVector.cend()));
        auto minDelayDuration = std::chrono::duration_cast<DoubleMilliSeconds>(
                *std::min_element(durationVector.cbegin(), durationVector.cend()));
        auto sumDelayDuration = std::chrono::duration_cast<DoubleMilliSeconds>(std::accumulate(
                durationVector.cbegin(), durationVector.cend(), ClockResolution(0)));
        double meanDelay = sumDelayDuration.count() / durationVector.size();

        using DoubleSeconds = std::chrono::duration<double>;
        auto totalDurationSec = std::chrono::duration_cast<DoubleSeconds>(totalDuration);
        double msgPerSec = durationVector.size() / totalDurationSec.count();

        std::cerr << "----- statistics -----" << std::endl;
        std::cerr << "totalDuration:\t" << totalDurationSec.count() << " [s]" << std::endl;
        std::cerr << "maxDelay:\t\t" << maxDelayDuration.count() << " [ms]" << std::endl;
        std::cerr << "minDelay:\t\t" << minDelayDuration.count() << " [ms]" << std::endl;
        std::cerr << "meanDelay:\t\t" << meanDelay << " [ms]" << std::endl;
        std::cerr << "msg/sec:\t\t" << msgPerSec << std::endl;
    }

    template <typename Function, typename... Args>
    static void runAndPrintAverage(const std::uint64_t runs,
                                   const std::string& name,
                                   Function&& fun,
                                   Args&&... args)
    {
        const auto startLoop = Clock::now();
        std::vector<ClockResolution> durationVector =
                benchmark(runs, std::forward<Function>(fun), std::forward<Args>(args)...);
        std::cerr << "Testcase: " << name << std::endl;

        const auto endLoop = Clock::now();
        printStatistics(
                durationVector, std::chrono::duration_cast<ClockResolution>(endLoop - startLoop));
    }
};

#endif // PERFORMANCE_TEST_H
