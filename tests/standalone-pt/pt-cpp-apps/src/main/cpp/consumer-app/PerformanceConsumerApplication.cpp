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

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/program_options.hpp>

#include <joynr/JoynrRuntime.h>
#include <joynr/Settings.h>
#include <joynr/WebSocketSettings.h>

#include <joynr/tests/DummyKeyChainParameters.h>
#include <joynr/tests/DummyKeychainImpl.h>

#include "../common/Enum.h"
#include "PerformanceConsumer.h"
#ifdef JOYNR_ENABLE_DLT_LOGGING
#include <dlt/dlt.h>
#endif // JOYNR_ENABLE_DLT_LOGGING

JOYNR_ENUM(SyncMode, (sync)(async));
JOYNR_ENUM(TestCase, (LOOKUP));

int main(int argc, char* argv[])
{
#ifdef JOYNR_ENABLE_DLT_LOGGING
    // Register app at the dlt-daemon for logging
    DLT_REGISTER_APP("JYPC", argv[0]);
#endif // JOYNR_ENABLE_DLT_LOGGING

    namespace po = boost::program_options;

    std::size_t calls;
    std::string domain;
    std::size_t maxInflightCalls;
    std::size_t repetition;
    std::string containerId;
    std::string gbidsString;
    std::vector<std::string> gbids;
    SyncMode syncMode;
    TestCase testCase;
    bool useKeyChain = false;
    const std::string ccUrlForTLS("wss://localhost:4243");
    joynr::tests::DummyKeyChainParameters keyChainInputParams;

    auto validateCalls = [](std::size_t value) {
        if (value == 0) {
            throw po::validation_error(
                    po::validation_error::invalid_option_value, "calls", std::to_string(value));
        }
    };

    auto validateMaxInflightCalls = [](std::size_t value) {
        if (value == 0) {
            throw po::validation_error(po::validation_error::invalid_option_value,
                                       "maxInflightCalls",
                                       std::to_string(value));
        }
    };

    auto validateRepetition = [](std::size_t value) {
        if (value == 0) {
            throw po::validation_error(po::validation_error::invalid_option_value,
                                       "repetition",
                                       std::to_string(value));
        }
    };

    auto validateContainerId = [](const std::string contId) {
        if (contId.empty()) {
            throw po::validation_error(
                    po::validation_error::invalid_option_value, "containerId", contId);
        }
    };

    auto validateGbids = [&gbids](const std::string& gbidsString) {
        if (gbidsString.empty()) {
            throw po::validation_error(
                    po::validation_error::invalid_option_value, "gbids", gbidsString);
        }
        boost::split(gbids, gbidsString, boost::is_any_of("\t,|' '"));
    };

    po::options_description desc("Available options");
    desc.add_options()("help,h", "produce help message")(
            "domain,d", po::value(&domain)->required(), "domain")(
            "calls,r", po::value(&calls)->required()->notifier(validateCalls), "number of calls")(
            "maxInflightCalls,m",
            po::value(&maxInflightCalls)->required()->notifier(validateMaxInflightCalls),
            "number of max inflight calls")(
            "repetition,r",
            po::value(&repetition)->required()->notifier(validateRepetition),
            "Repetition number of the performance test which exectues")(
            "containerId,c",
            po::value(&containerId)->required()->notifier(validateContainerId),
            "container Id which calls the performance test")(
            "gbids,b", po::value(&gbidsString)->required()->notifier(validateGbids), "gbids")(
            "testCase,t", po::value(&testCase)->default_value(TestCase::LOOKUP), "lookup")(
            "syncMode,s", po::value(&syncMode)->required(), "sync|async")(
            "useKeychain",
            po::value(&useKeyChain)->default_value(false),
            "Should KeyChain be used? Default: false.")(
            "root-certificate",
            po::value(&keyChainInputParams.rootCertFileName),
            "Root certificate in PEM encoded format.")(
            "pub-certificate",
            po::value(&keyChainInputParams.pubCertFileName),
            "Public certificate in PEM encoded format.")(
            "private-key",
            po::value(&keyChainInputParams.privKeyFileName),
            "Private key in PEM encoded format.")(
            "private-key-pwd",
            po::value(&keyChainInputParams.privKeyPassword)->default_value(""),
            "Passsword of private key. Default: empty string.");

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return EXIT_FAILURE;
        }

        po::notify(vm);

        if (calls < maxInflightCalls) {
            std::cout << "Number of calls should be greater than or equal to the max number of "
                         "inflight calls \n";
            return EXIT_FAILURE;
        }

        boost::filesystem::path appFilename = boost::filesystem::path(argv[0]);
        std::string appDirectory =
                boost::filesystem::system_complete(appFilename).parent_path().string();
        auto joynrSettings = std::make_unique<joynr::Settings>(
                (appDirectory + "/resources/performancetest-consumer.settings"));

        std::shared_ptr<joynr::IKeychain> keyChain;

        if (useKeyChain) {
            keyChain = joynr::tests::DummyKeychainImpl::createFromPEMFiles(keyChainInputParams);
            // also change settings file accordingly
            joynr::WebSocketSettings wsSettings(*joynrSettings);
            wsSettings.setClusterControllerMessagingUrl(ccUrlForTLS);
        }

        std::shared_ptr<joynr::JoynrRuntime> runtime(
                joynr::JoynrRuntime::createRuntime(std::move(joynrSettings), std::move(keyChain)));

        std::unique_ptr<joynr::IPerformanceConsumer> consumer;

        if (syncMode == SyncMode::sync) {
            consumer = std::make_unique<joynr::SyncConsumer>(std::move(runtime),
                                                             calls,
                                                             domain,
                                                             maxInflightCalls,
                                                             repetition,
                                                             containerId,
                                                             std::move(gbids));
        } else {
            consumer = std::make_unique<joynr::ASyncConsumer>(std::move(runtime),
                                                              calls,
                                                              domain,
                                                              maxInflightCalls,
                                                              repetition,
                                                              containerId,
                                                              std::move(gbids));
        }

        switch (testCase) {
        case TestCase::LOOKUP:
            std::string participantId =
                    "lrc.erqw.sde.vbgf.as.nmc.tt.yte.bgtrfvcd.OtmvqaZwertvbgfdwqBTG";
            std::string interfaceName = "go/wef/yg/sdf/ghtrqazs/OtmvqaZwertvbgfdwqBTG";
            std::vector<std::string> domains{"gfz.abcd"};
            // consumer->runLookup(participantId);
            consumer->runLookup(domains, interfaceName);
            break;
        }
    } catch (const std::exception& e) {
        std::cerr << e.what();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
