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

#include <boost/program_options.hpp>

#include "joynr/system/RoutingTypes/Address.h"

#include "ShortCircuitTest.h"

#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

#include "../common/Enum.h"
JOYNR_ENUM(TestCase, (SEND_STRING)(SEND_BYTEARRAY)(SEND_STRUCT));

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYPS", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    namespace po = boost::program_options;

    std::size_t runs;
    TestCase testCase;

    auto validateRuns = [](std::size_t value) {
        if (value == 0) {
            throw po::validation_error(
                    po::validation_error::invalid_option_value, "runs", std::to_string(value));
        }
    };

    po::options_description desc("Available options");
    desc.add_options()("help,h", "produce help message")(
            "runs,r", po::value(&runs)->required()->notifier(validateRuns), "number of runs")(
            "testCase,t",
            po::value(&testCase)->required(),
            "SEND_STRING|SEND_BYTEARRAY|SEND_STRUCT");

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_FAILURE;
        }

        ShortCircuitTest test(runs);

        switch (testCase) {
        case TestCase::SEND_BYTEARRAY:
            test.roundTripByteArray(10000);
            test.roundTripByteArray(100000);
            break;
        case TestCase::SEND_STRING:
            test.roundTripString(100);
            break;
        case TestCase::SEND_STRUCT:
            test.roundTripStruct(100);
            break;
        }

    } catch (const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
