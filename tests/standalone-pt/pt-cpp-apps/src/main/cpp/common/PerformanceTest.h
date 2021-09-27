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

#ifndef PERFORMANCE_TEST_H
#define PERFORMANCE_TEST_H

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <numeric>
#include <sstream>
#include <utility>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"

using Clock = std::chrono::steady_clock;
using ClockResolution = std::chrono::microseconds;

struct PerformanceTest
{
    template <typename T1,
              typename T2,
              typename T3,
              typename T4,
              typename T5,
              typename T6,
              typename T7,
              typename T8,
              typename T9>
    static void writeDataToFile(const std::string& csvFile,
                                T1 column1,
                                T2 column2,
                                T3 column3,
                                T4 column4,
                                T5 column5,
                                T6 column6,
                                T7 column7,
                                T8 column8,
                                T9 column9)
    {
        std::mutex logMutex;
        auto isFileExists = [&csvFile] () {
            return static_cast<bool>(std::ifstream(csvFile));
        };

        auto writeData = [&csvFile, &logMutex] (
                const auto& column1,
                const auto& column2,
                const auto& column3,
                const auto& column4,
                const auto& column5,
                const auto& column6,
                const auto& column7,
                const auto& column8,
                const auto& column9) {
            std::lock_guard<std::mutex> csvLock(logMutex);
            std::fstream file;
            file.open (csvFile, std::ios::out | std::ios::app);
            if (!file) {
                std::cerr << "Failed to create/open file" << csvFile << "\n";
                return;
            }
            file << "\"" << column1 << "\",";
            file << "\"" << column2 << "\",";
            file << "\"" << column3 << "\",";
            file << "\"" << column4 << "\",";
            file << "\"" << column5 << "\",";
            file << "\"" << column6 << "\",";
            file << "\"" << column7 << "\",";
            file << "\"" << column8 << "\",";
            file << "\"" << column9 << "\"";
            file << std::endl;
        };

        // write headers only once
        if(!isFileExists()) {
            writeData("ContainerId",
                      "Repetition",
                      "Calls",
                      "Timestamp[utc]",
                      "TotalResponseTime[s]",
                      "MaxResponseTime[ms]",
                      "MinResponseTime[ms]",
                      "AverageResponseTime[ms]",
                      "RequestPerSec");
        }

        writeData(column1, column2, column3, column4, column5, column6, column7, column8, column9);
    }

    static void printStatistics(std::vector<ClockResolution> durationVector,
                                ClockResolution totalDuration,
                                std::size_t repetition,
                                std::size_t calls,
                                const std::string& containerId)
    {
        using DoubleMilliSeconds = std::chrono::duration<double, std::milli>;
        auto maxResponseTime = std::chrono::duration_cast<DoubleMilliSeconds>(
                *std::max_element(durationVector.cbegin(), durationVector.cend()));
        auto minResponseTime = std::chrono::duration_cast<DoubleMilliSeconds>(
                *std::min_element(durationVector.cbegin(), durationVector.cend()));
        auto sumDelayDuration = std::chrono::duration_cast<DoubleMilliSeconds>(std::accumulate(
                durationVector.cbegin(), durationVector.cend(), ClockResolution(0)));
        double averageResponseTime = sumDelayDuration.count() / durationVector.size();

        using DoubleSeconds = std::chrono::duration<double>;
        auto totalResponseTimeSec = std::chrono::duration_cast<DoubleSeconds>(totalDuration);
        double requestPerSec = durationVector.size() / totalResponseTimeSec.count();
        auto getTimeStamp = [] () {
            boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
            std::stringstream ss;
            ss << static_cast<int>(now.date().year())
            << "-" << std::setfill('0') << std::setw(2) << (now.date().month().as_number())
            << "-" << std::setfill('0') << std::setw(2) << now.date().day() << " "
            << std::setfill('0') << std::setw(2) << now.time_of_day().hours() << ":"
            << std::setfill('0') << std::setw(2) << now.time_of_day().minutes() << ":"
            << std::setfill('0') << std::setw(2) << now.time_of_day().seconds() << "."
            << now.time_of_day().total_milliseconds();
            return ss.str();
        };

        std::cerr << "----- statistics -----" << std::endl;
        std::cerr << "ContainerId:\t" << containerId << std::endl;
        std::cerr << "Repetition:\t" << repetition << std::endl;
        std::cerr << "Calls:\t" << calls << std::endl;
        std::cerr << "Timestamp[utc]:\t" << getTimeStamp() << std::endl;
        std::cerr << "TotalResponseTime:\t" << totalResponseTimeSec.count() << " [s]" << std::endl;
        std::cerr << "MaxResponseTime:\t\t" << maxResponseTime.count() << " [ms]" << std::endl;
        std::cerr << "MinResponseTime:\t\t" << minResponseTime.count() << " [ms]" << std::endl;
        std::cerr << "AverageResponseTime:\t\t" << averageResponseTime << " [ms]" << std::endl;
        std::cerr << "RequestPerSec:\t\t" << requestPerSec << std::endl;

        const std::string csvFile = "/home/joynr/build/results_cont_"+containerId+".csv";
        std::cerr << "Writing results to: " << csvFile << "\n";
        PerformanceTest::writeDataToFile(csvFile,
                    containerId,
                    repetition,
                    calls,
                    getTimeStamp(),
                    totalResponseTimeSec.count(),
                    maxResponseTime.count(),
                    minResponseTime.count(),
                    averageResponseTime,
                    requestPerSec
                    );
    }
};

#endif // PERFORMANCE_TEST_H
