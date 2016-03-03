/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

#include <iomanip>
#include <iostream>
#include <chrono>
#include <utility>

template <std::size_t Runs = 1000>
struct PerformanceTest
{
    /**
     * @brief executes a given function @param func with arguments @param args
     * @returns average duration of a function call in milliseconds
     */
    template <typename Function, typename... Args>
    double benchmark(Function&& fun, Args&&... args)
    {
        using Clock = std::chrono::steady_clock;
        using ClockResolution = std::chrono::microseconds;
        using ResultType = decltype(fun(args...));

        const auto start = Clock::now();

        executeBenchmark(std::forward<Function>(fun),
                         std::forward<Args>(args)...,
                         std::is_void<ResultType>{});

        return std::chrono::duration_cast<ClockResolution>(Clock::now() - start).count() /
               static_cast<double>(Runs * 1000);
    }

    template <typename Function, typename... Args>
    void executeBenchmark(Function&& fun, Args&&... args, std::true_type)
    {
        for (std::size_t i = 0; i < Runs; ++i) {
            fun(std::forward<Args>(args)...);
        }
    }

    template <typename Function, typename... Args>
    void executeBenchmark(Function&& fun, Args&&... args, std::false_type)
    {
        using ResultType = decltype(fun(args...));
        // In order to prevent compiler optimization, the result of the function call
        // is stored in a volatile variable.
        for (std::size_t i = 0; i < Runs; ++i) {
            volatile ResultType result = fun(std::forward<Args>(args)...);
        }
    }

    template <typename Function, typename... Args>
    void runAndPrintAverage(const std::string& name, Function&& fun, Args&&... args)
    {
        double averageDuration =
                benchmark(std::forward<Function>(fun), std::forward<Args>(args)...);
        std::cout << std::setprecision(3) << name << " => average time: " << averageDuration
                  << " ms => " << (1000 / averageDuration) << " msg/sec" << std::endl;
    }
};

#endif // PERFORMANCE_TEST_H
